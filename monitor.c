/*
 * Note: the interface must typically be brought down before changing
 * type, and back up afterward. This example uses libnl for the nl80211
 * type change and a plain ioctl (SIOCSIFFLAGS) for the up/down toggle,
 * since that's simpler than doing it via rtnetlink/libnl as well.
 */

#include "monitor.h"
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

/* Bring interface up or down via a basic ioctl */
static int set_iface_up(const char *ifname, bool up) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
        close(sock);
        return -1;
    }

    if (up) ifr.ifr_flags |= IFF_UP;
    else ifr.ifr_flags &= ~IFF_UP;

    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        close(sock);
        return -1;
    }

    close(sock);
    return 0;
}

/*
 * Generic helper: set the interface type via nl80211.
 * type = NL80211_IFTYPE_MONITOR or NL80211_IFTYPE_STATION (etc.)
 */
static int nl80211_set_iftype(const char *ifname, enum nl80211_iftype type) {
    struct nl_sock *sk;
    struct nl_msg *msg;
    int nl80211_id;
    int ifindex;
    int ret;

    ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
        fprintf(stderr, "Unknown interface: %s\n", ifname);
        return -1;
    }

    sk = nl_socket_alloc();
    if (!sk) {
        fprintf(stderr, "Failed to allocate netlink socket\n");
        return -1;
    }

    if (genl_connect(sk)) {
        fprintf(stderr, "Failed to connect to generic netlink\n");
        nl_socket_free(sk);
        return -1;
    }

    nl80211_id = genl_ctrl_resolve(sk, "nl80211");
    if (nl80211_id < 0) {
        fprintf(stderr, "nl80211 not found\n");
        nl_socket_free(sk);
        return -1;
    }

    msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Failed to allocate netlink message\n");
        nl_socket_free(sk);
        return -1;
    }

    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id, 0, 0, NL80211_CMD_SET_INTERFACE, 0);

    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ifindex);
    NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, type);

    ret = nl_send_auto_complete(sk, msg);
    if (ret < 0) {
        fprintf(stderr, "nl_send_auto_complete failed: %d\n", ret);
        goto out;
    }

    ret = nl_wait_for_ack(sk);
    if (ret < 0) {
        fprintf(stderr, "nl_wait_for_ack failed: %d\n", ret);
        goto out;
    }

    ret = 0;

    out:
    nlmsg_free(msg);
    nl_socket_free(sk);
    return ret;

    nla_put_failure:
    fprintf(stderr, "NLA_PUT failure\n");
    nlmsg_free(msg);
    nl_socket_free(sk);
    return -1;
}

/* Turn monitor mode ON for the given interface */
int monitor_mode_on(const char *ifname) {
    printf("[*] Enabling monitor mode on %s\n", ifname);

    if (set_iface_up(ifname, 0) < 0) {
        fprintf(stderr, "Failed to bring %s down\n", ifname);
        return -1;
    }

    if (nl80211_set_iftype(ifname, NL80211_IFTYPE_MONITOR) < 0) {
        fprintf(stderr, "Failed to set %s to monitor mode\n", ifname);
        return -1;
    }

    if (set_iface_up(ifname, 1) < 0) {
        fprintf(stderr, "Failed to bring %s up\n", ifname);
        return -1;
    }

    printf("[+] %s is now in monitor mode\n", ifname);
    return 0;
}

/* Turn monitor mode OFF, restoring managed (station) mode */
int monitor_mode_off(const char *ifname) {
    printf("[*] Disabling monitor mode on %s\n", ifname);

    if (set_iface_up(ifname, 0) < 0) {
        fprintf(stderr, "Failed to bring %s down\n", ifname);
        return -1;
    }

    if (nl80211_set_iftype(ifname, NL80211_IFTYPE_STATION) < 0) {
        fprintf(stderr, "Failed to set %s to managed mode\n", ifname);
        return -1;
    }

    if (set_iface_up(ifname, 1) < 0) {
        fprintf(stderr, "Failed to bring %s up\n", ifname);
        return -1;
    }

    printf("[+] %s is back in managed mode\n", ifname);
    return 0;
}