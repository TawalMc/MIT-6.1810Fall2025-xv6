#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int main(int argc, char *argv[])
{
	// Your code here.
	char pre_word[] = "This may help.";
	char *brk = sbrk(4096 * 8 * 2);
	if (brk < 0)
	{
		printf("sbrk: failed\n");
		exit(0);
	}

	int start = 0;
	for (int i = 0; i < 4096; i++)
	{
		if ('T' == brk[i] && start != 1)
		{
			start = 1;
			for (int j = i; j < i + strlen(pre_word); j++)
			{
				if (brk[j] != pre_word[j - i])
				{
					break;
				}
				if (j - i + 1 == strlen(pre_word))
				{
					i += strlen(pre_word) + 2;
					start = 1;
					break;
				}
			}
		}

		if (start == 1)
		{
			/* code */
			printf("%c", brk[i]);
		}

	}
	printf("\n");

	exit(1);
}
