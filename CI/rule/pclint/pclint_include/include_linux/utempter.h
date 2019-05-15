void addToUtmp(const char *pty, const char *hostname, int cmdfd);
void removeFromUtmp(void);	/* removes the last pty added to utmp */
void removeLineFromUtmp(const char * pty, int fd);
