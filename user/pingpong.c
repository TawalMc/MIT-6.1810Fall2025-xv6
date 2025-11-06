#include "kernel/types.h"
#include "user/user.h"

void main(int argc, char *argv[])
{
	int fds[2];
	char *buf = "b";
	int pid = fork();

	//   pause(10);

	pipe(fds);

	if (pid < 0)
	{
		fprintf(2, "an error occured when creating new child \n");
		exit(1);
	}
	else if (pid == 0)
	{
		// child
		close(fds[1]);

		read(fds[0], buf, 1);
		fprintf(1, "%d: received ping \n", getpid());

		write(fds[1], buf, 1);
		exit(0);
	}
	else
	{
		// parent
		write(fds[1], buf, 1);
		close(fds[1]);
		wait((int *)0);

		read(fds[0], buf, 1);
		fprintf(1, "%d: received pong \n", getpid());
		close(fds[0]);
		exit(0);
	}

	fprintf(1, "parent: yes");

	exit(0);
}
