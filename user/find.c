#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
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
	// memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
	buf[sizeof(buf) - 1] = '\0';
	return buf;
}

void find(char *path, char *filename)
{
	fprintf(stdin, "-- path: %s --\n", path);

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
		fprintf(stdin, "name: %s\n", de.name);
		if (de.inum == 0)
			continue;
		memmove(p, de.name, DIRSIZ);
		p[DIRSIZ] = 0;
		if (stat(buf, &st) < 0)
		{
			fprintf(stderr, "find: cannot stat %s\n", buf);
			continue;
		}

		if (strcmp(fmtname(buf), ".") == 0 || strcmp(fmtname(buf), "..") == 0)
			continue;

		fprintf(stdout, "%s: %s: %d\n", buf, fmtname(buf), st.type);
		if (st.type == T_DIR)
		{
			find(buf, filename);
		}
	}

	close(fd);
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, "find usage: find [path] filename\n");
		exit(1);
	}
	// if (argc == 2)
	// {
	// 	find(".", argv[1]);
	// 	exit(0);
	// }
	// for (int i = 1; i < count; i++)
	// {
	// 	/* code */
	// }

	find(argv[1], argv[2]);

	exit(0);
}