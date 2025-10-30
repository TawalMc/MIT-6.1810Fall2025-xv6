#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAX_BUF_SIZE 40

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "sixfive usage: sixfive [filenames...] \n");
		exit(1);
	}

	for (int i = 1; i < argc; i++)
	{
		int fd = open(argv[i], O_RDONLY);

		if (fd == -1)
		{
			fprintf(stderr, "cannot open file %s \n", argv[i]);
			continue;
		}

		char valid_char[] = " -\r\t\n./,";
		char c;

		char buf[MAX_BUF_SIZE];
		int n = 0;
		int num = 0;
		int buf_len = 0;
		bool start_valid = true;

		memset(buf, 0, MAX_BUF_SIZE);
		buf[0] = '\0';

		while ((n = read(fd, &c, 1)) >= 0)
		{
			char* is_valid = strchr(valid_char, c);
			if (is_valid || n == 0)
			{
				start_valid = true;
				if (buf[0] != '\0')
				{
					//_sizeof = 0;
					buf[buf_len] = '\0';
					num = atoi(buf);
					if (num % 5 == 0 || num % 6 == 0)
					{
						fprintf(stdout, "%d \n", num);
					}
				}

				memset(buf, 0, MAX_BUF_SIZE);
				buf[0] = '\0';
				buf_len = 0;
			}

			if (!is_valid)
			{
				if ('0' <= c && c <= '9' && start_valid)
				{
					buf[buf_len] = c;
					buf_len++;
				}
				else
				{
					start_valid = false;
					if (buf[0] != '\0')
					{
						memset(buf, 0, MAX_BUF_SIZE);
						buf[0] = '\0';
						buf_len = 0;
					}
				}
			}

			if (n == 0)
				break;
		}
	}

	exit(0);
}
