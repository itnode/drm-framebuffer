#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "framebuffer.h"

#define SOCKET_PATH "/tmp/drmfb.sock"

int main(int argc, char** argv) {
    char *dri_device = NULL;
    char *connector = NULL;
    int c;
    int selected_resolution = -1;
    int ret;

    opterr = 0;
    while ((c = getopt (argc, argv, "d:c:s:h")) != -1) {
        switch (c) {
        case 'd':
            dri_device = optarg;
            break;
        case 'c':
            connector = optarg;
            break;
        case 's':
            selected_resolution = atoi(optarg);
            break;
        case 'h':
            printf("Usage: %s -d <dri_device> -c <connector>\n", argv[0]);
            return 1;
        default:
            break;
        }
    }
    if (!dri_device || !connector) {
        printf("Device und Connector müssen angegeben werden!\n");
        return 1;
    }

    struct framebuffer fb;
    memset(&fb, 0, sizeof(fb));
    if (get_framebuffer(dri_device, connector, &fb, selected_resolution) != 0) {
        printf("Framebuffer konnte nicht initialisiert werden!\n");
        return 1;
    }

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    unlink(SOCKET_PATH);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    listen(server_fd, 1);
    printf("drmfb_daemon läuft. Socket: %s\n", SOCKET_PATH);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;
        size_t total_read = 0;
        while (total_read < fb.dumb_framebuffer.size) {
            ssize_t sz = read(client_fd, &fb.data[total_read], fb.dumb_framebuffer.size - total_read);
            if (sz <= 0) break;
            total_read += sz;
        }
        // Synchronisiere das Bild mit dem Display
        drmSetMaster(fb.fd);
        drmModeSetCrtc(fb.fd, fb.crtc->crtc_id, fb.buffer_id, 0, 0, &fb.connector->connector_id, 1, fb.resolution);
        drmDropMaster(fb.fd);
        close(client_fd);
    }
    close(server_fd);
    unlink(SOCKET_PATH);
    release_framebuffer(&fb);
    return 0;
}
