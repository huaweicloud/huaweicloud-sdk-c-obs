#ifndef SIGNAL_HANDLE_H
#define SIGNAL_HANDLE_H
#if defined __GNUC__ || defined LINUX
void set_sigaction_for_sigpipe();
void unset_sigaction_for_sigpipe();
#endif
#endif
