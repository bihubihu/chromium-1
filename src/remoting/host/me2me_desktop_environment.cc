// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/me2me_desktop_environment.h"

#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "media/video/capture/screen/screen_capturer.h"
#include "remoting/host/client_session_control.h"
#include "remoting/host/desktop_resizer.h"
#include "remoting/host/host_window.h"
#include "remoting/host/host_window.h"
#include "remoting/host/host_window_proxy.h"
#include "remoting/host/local_input_monitor.h"
#include "remoting/host/resizing_host_observer.h"
#include "remoting/host/screen_controls.h"

#if defined(OS_POSIX)
#include <sys/types.h>
#include <unistd.h>
#endif  // defined(OS_POSIX)

namespace remoting {

Me2MeDesktopEnvironment::~Me2MeDesktopEnvironment() {
  DCHECK(caller_task_runner()->BelongsToCurrentThread());
}

scoped_ptr<ScreenControls> Me2MeDesktopEnvironment::CreateScreenControls() {
  DCHECK(caller_task_runner()->BelongsToCurrentThread());

  return scoped_ptr<ScreenControls>(
      new ResizingHostObserver(DesktopResizer::Create()));
}

scoped_ptr<media::ScreenCapturer>
Me2MeDesktopEnvironment::CreateVideoCapturer() {
  DCHECK(caller_task_runner()->BelongsToCurrentThread());

#if defined(OS_LINUX)
  return media::ScreenCapturer::CreateWithXDamage(true);
#else  // !defined(OS_LINUX)
  return media::ScreenCapturer::Create();
#endif  // !defined(OS_LINUX)
}

Me2MeDesktopEnvironment::Me2MeDesktopEnvironment(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control,
    const UiStrings& ui_strings)
    : BasicDesktopEnvironment(caller_task_runner,
                              input_task_runner,
                              ui_task_runner,
                              client_session_control,
                              ui_strings) {
  DCHECK(caller_task_runner->BelongsToCurrentThread());

  // Create the local input monitor.
  local_input_monitor_ = LocalInputMonitor::Create(caller_task_runner,
                                                   input_task_runner,
                                                   ui_task_runner,
                                                   client_session_control);

  // The host UI should be created on the UI thread.
  bool want_user_interface = true;
#if defined(OS_LINUX)
  want_user_interface = false;
#elif defined(OS_MACOSX)
  // Don't try to display any UI on top of the system's login screen as this
  // is rejected by the Window Server on OS X 10.7.4, and prevents the
  // capturer from working (http://crbug.com/140984).

  // TODO(lambroslambrou): Use a better technique of detecting whether we're
  // running in the LoginWindow context, and refactor this into a separate
  // function to be used here and in CurtainMode::ActivateCurtain().
  want_user_interface = getuid() != 0;
#endif  // defined(OS_MACOSX)

  // Create the disconnect window.
  if (want_user_interface) {
    disconnect_window_ = HostWindow::CreateDisconnectWindow(ui_strings);
    disconnect_window_.reset(new HostWindowProxy(
        caller_task_runner,
        ui_task_runner,
        disconnect_window_.Pass()));
    disconnect_window_->Start(client_session_control);
  }
}

Me2MeDesktopEnvironmentFactory::Me2MeDesktopEnvironmentFactory(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    const UiStrings& ui_strings)
    : BasicDesktopEnvironmentFactory(caller_task_runner,
                                     input_task_runner,
                                     ui_task_runner,
                                     ui_strings) {
}

Me2MeDesktopEnvironmentFactory::~Me2MeDesktopEnvironmentFactory() {
}

scoped_ptr<DesktopEnvironment> Me2MeDesktopEnvironmentFactory::Create(
    base::WeakPtr<ClientSessionControl> client_session_control) {
  DCHECK(caller_task_runner()->BelongsToCurrentThread());

  return scoped_ptr<DesktopEnvironment>(
      new Me2MeDesktopEnvironment(caller_task_runner(),
                                  input_task_runner(),
                                  ui_task_runner(),
                                  client_session_control,
                                  ui_strings()));
}

}  // namespace remoting
