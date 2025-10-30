#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(2, "sixfive usage: sixfive [filenames...]");
		exit(1);
	}

	for (int i = 1; i < argc; i++)
	{
		int fd = open(argv[i], O_RDONLY);

		char valid_char[] = " -\r\t\n./,";
		char c;
		const int max = 40;

		char buf[max];
		int n = 0;
		int num = 0;
		int _sizeof = 0;

		memset(buf, 0, sizeof(max));
		buf[0] = '\0';

		while ((n = read(fd, &c, 1)) >= 0)
		{
			if (strchr(valid_char, c) || n == 0)
			{
				if (buf[0] != '\0')
				{
					//_sizeof = 0;
					buf[_sizeof] = '\0';
					num = atoi(buf);
					if (num % 5 == 0 || num % 6 == 0)
					{
						fprintf(1, "%d \n", num);
					}
				}

				memset(buf, 0, sizeof(max));
				buf[0] = '\0';
				_sizeof = 0;
			}

			if (48 <= c && c <= 57)
			{
				buf[_sizeof] = c;
				_sizeof++;
			}

			if (n == 0)
				break;
		}
	}

	exit(0);
}
