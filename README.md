
# fanotify example

Simple example how to use fanotify with the [new capabilities](https://lkml.org/lkml/2019/3/1/400) in Linux 5.1, like `FAN_DELETE`.

I was having similar problems as [this bug](https://bugzilla.kernel.org/show_bug.cgi?id=205639), bit it comes to using

On my openSUSE system, `fanotify(7)` is outdated, claiming that deletes are not supported, even if it runs Kernel 5.8.4:

> In particular, there is no support for create, delete, and move events. (See  inotify(7) for details of an API that does notify those events.

Note that [this link](https://man7.org/linux/man-pages/man7/fanotify.7.html) is also outdated.

However, the man page for `fanotify_mark(2)` reports correctly:

> FAN_DELETE (since Linux 5.1)
>  Create an event when a file or directory  has  been  deleted  in  a
>  marked  parent directory.  An fanotify file descriptor created with
>  FAN_REPORT_FID is required.

I have [reported this to the](https://bugzilla.kernel.org/show_bug.cgi?id=209247) kernel manpages with a proposed patch:

```diff
diff --git a/man7/fanotify.7 b/man7/fanotify.7
index c3d40b56d..9a52dbc4c 100644
--- a/man7/fanotify.7
+++ b/man7/fanotify.7
@@ -29,11 +29,14 @@ fanotify \- monitoring filesystem events
 The fanotify API provides notification and interception of
 filesystem events.
 Use cases include virus scanning and hierarchical storage management.
-Currently, only a limited set of events is supported.
-In particular, there is no support for create, delete, and move events.
-(See
-.BR inotify (7)
-for details of an API that does notify those events.)
+
+Note that events like create, delete, and move require a fanotify
+group that identifies filesystem objects by file handles.
+This is achieved with the flag
+.BR FAN_REPORT_FID " (since Linux 5.1)"
+.\" commit 235328d1fa4251c6dcb32351219bb553a58838d2
+of
+.BR fanotify_init (2).
 .PP
 Additional capabilities compared to the
 .BR inotify (7)
```

This example shows how to use `FAN_REPORT_FID` and the new events like `FAN_DELETE`.
