#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fanotify.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    int fan;
    int mount_fd, event_fd;
    char buf[4096];
    char fdpath[32];
    char path[PATH_MAX + 1];
    struct file_handle *file_handle;

    ssize_t buflen, linklen;
    struct fanotify_event_metadata *metadata;
    struct fanotify_event_info_fid *fid;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dir\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    mount_fd = open(argv[1], O_DIRECTORY | O_RDONLY);
    if (mount_fd == -1) {
        perror(argv[1]);
        exit(EXIT_FAILURE);
    }

    fan = fanotify_init(FAN_CLASS_NOTIF | FAN_REPORT_FID, O_RDWR);
    if(fan == -1) {
        perror("fanotify_init");
        exit(EXIT_FAILURE);
    }

    int ret = fanotify_mark(fan,
                            FAN_MARK_ADD,
                            FAN_CREATE | FAN_DELETE | FAN_MODIFY | FAN_EVENT_ON_CHILD,
                            AT_FDCWD,
                            argv[1]
                            );
    if(ret == -1) {
        perror("fanotify_mark");
        exit(EXIT_FAILURE);
    }

    while(1) {
        buflen = read(fan, buf, sizeof(buf));
        metadata = (struct fanotify_event_metadata*)&buf;

        for (; FAN_EVENT_OK(metadata, buflen); metadata = FAN_EVENT_NEXT(metadata, buflen)) {
            fid = (struct fanotify_event_info_fid *) (metadata + 1);
            file_handle = (struct file_handle *) fid->handle;

            /* Ensure that the event info is of the correct type */

            if (fid->hdr.info_type != FAN_EVENT_INFO_TYPE_FID) {

                fprintf(stderr, "Received unexpected event info type.\n");
                exit(EXIT_FAILURE);
            }

            if (metadata->mask == FAN_CREATE)
                printf("FAN_CREATE");

            if (metadata->mask == FAN_DELETE)
                printf("FAN_DELETE");

            if (metadata->mask == FAN_MODIFY)
                printf("FAN_MODIFY");

            event_fd = open_by_handle_at(mount_fd, file_handle, O_RDONLY);
            if (event_fd == -1) {
                if (errno == ESTALE) {
                    printf("File handle is no longer valid. "
                           "File has been deleted\n");
                    continue;
                } else {
                    perror("open_by_handle_at");
                    exit(EXIT_FAILURE);
                }
            }

            sprintf(fdpath, "/proc/self/fd/%d", event_fd);
            linklen = readlink(fdpath, path, sizeof(path) - 1);
            if (linklen == -1) {
                perror("readlink");
            }
            path[linklen] = '\0';
            printf("%s\n", path);

            close(metadata->fd);
            metadata = FAN_EVENT_NEXT(metadata, buflen);
        }
    }
}
