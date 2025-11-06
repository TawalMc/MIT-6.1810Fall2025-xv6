#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

char *
fmtname(char *path)
{
	static char buf[DIRSIZ + 1];
	char *p;

	memset(buf, '\0', sizeof(buf));
	// Find first character after last slash.
	for (p = path + strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	// Return blank-padded name.
	if (strlen(p) >= DIRSIZ)
		return p;
	memmove(buf, p, strlen(p));
	buf[sizeof(buf) - 1] = '\0';
	return buf;
}

void find(char *path, char *filename, int count_cmd, char *cmd[])
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "find: cannot open %s\n", path);
		return;
	}

	struct stat st;
	if (fstat(fd, &st) < 0)
	{
		fprintf(stderr, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	if (st.type != T_DIR)
	{
		fprintf(stderr, "find: path is not a directory\n");
		close(fd);
		return;
	}

	char buf[512], *p;
	if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
	{
		fprintf(stderr, "find: path too long\n");
		close(fd);
		return;
	}

	strcpy(buf, path);
	p = buf + strlen(buf);
	*p++ = '/';

	struct dirent de;
	while (read(fd, &de, sizeof(de)) == sizeof(de))
	{
		if (de.inum == 0)
			continue;
		memmove(p, de.name, DIRSIZ);
		p[DIRSIZ] = 0;
		if (stat(buf, &st) < 0)
		{
			fprintf(stderr, "find: cannot stat %s\n", buf);
			continue;
		}

		char *base_name = fmtname(buf);
		if (strcmp(base_name, ".") == 0 || strcmp(base_name, "..") == 0)
			continue;

		if (st.type == T_FILE && strcmp(base_name, filename) == 0)
		{
			if (count_cmd == 0)
			{
				fprintf(stdout, "%s\n", buf);
			}
			else
			{
				for (int i = 0; i < count_cmd; i++)
				{
					fprintf(stdin, "%s \n", cmd[i]);
				}

				int pid = fork();
				if (pid < 0)
				{
					fprintf(2, "an error occured when creating new child \n");
					exit(1);
				}
				else if (pid == 0)
				{
					exec(cmd[0], cmd + 1);
					exit(0);
				}
				else
				{
					wait((int *)0);
				}
			}
		}

		if (st.type == T_DIR)
		{
			find(buf, filename, count_cmd, cmd);
		}
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	if (argc <= 2)
	{
		fprintf(stderr, "find usage: find [path] filename\n");
		exit(1);
	}

	if (argc >= MAXARG)
	{
		fprintf(stderr, "find: to much args\n");
		exit(1);
	}

	if (strlen(argv[2]) > MAXPATH)
	{
		fprintf(stderr, "find: filename too long\n");
		exit(1);
	}

	if (argc == 3)
	{
		char *null_data[] = {0};
		find(argv[1], argv[2], 0, null_data);
		exit(0);
	}

	// char cmd[512], *p;
	// memset(cmd, '\0', sizeof(cmd));
	// p = cmd;
	//$ find . wc -exec echo hi
	if (argc > 3 && strcmp(argv[3], "-exec") == 0)
	{
		// for (int i = 4; i < argc; i++)
		// {
		// 	strcpy(p, argv[i]);
		// 	p = p + strlen(argv[i]);
		// 	*p = ' ';
		// 	p++;
		// }
		// cmd[strlen(cmd) - 1] = '\0';
		char *cmd = argv;


		find(argv[1], argv[2], argc - 3, cmd);
	}

	exit(0);
}