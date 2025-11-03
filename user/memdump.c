#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data);

char *memdump_c(char *data);
char *memdump_i(char *data);
char *memdump_h(char *data);
char *memdump_p(char *data);
char *memdump_S(char *data);
char *memdump_s(char *data);

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("Example 1:\n");
		int a[2] = {61810, 2025};
		memdump("ii", (char *)a);

		printf("Example 2:\n");
		memdump("S", "a string");
		// exit(0);

		printf("Example 3:\n");
		char *s = "another";
		memdump("s", (char *)&s);
		// exit(0);

		struct sss
		{
			char *ptr;
			int num1;
			short num2;
			char byte;
			char bytes[8];
		} example;

		example.ptr = "hello";
		example.num1 = 1819438967;
		example.num2 = 100;
		example.byte = 'z';
		strcpy(example.bytes, "xyzzy");

		printf("Example 4:\n");
		memdump("pihcS", (char *)&example);

		printf("Example 5:\n");
		memdump("sccccc", (char *)&example);
	}
	else if (argc == 2)
	{
		// format in argv[1], up to 512 bytes of data from standard input.
		char data[512];
		int n = 0;
		memset(data, '\0', sizeof(data));
		while (n < sizeof(data))
		{
			int nn = read(0, data + n, sizeof(data) - n);
			if (nn <= 0)
				break;
			n += nn;
		}
		memdump(argv[1], data);
	}
	else
	{
		printf("Usage: memdump [format]\n");
		exit(1);
	}
	exit(0);
}

void memdump(char *fmt, char *data)
{
	for (; *fmt != '\0'; fmt++)
	{
		switch (*fmt)
		{
		case 'c':
			data = memdump_c(data);
			break;
		case 'i':
			data = memdump_i(data);
			break;
		case 'h':
			data = memdump_h(data);
			break;
		case 'p':
			data = memdump_p(data);
			break;
		case 'S':
			data = memdump_S(data);
			break;
		case 's':
			data = memdump_s(data);
			break;

		default:
			break;
		}

		printf("\n");
	}
}

char *memdump_c(char *data)
{
	printf("%c", *data);
	return data + sizeof(uint8);
}

char *memdump_i(char *data)
{
	printf("%d", *(uint32 *)data);
	return data + sizeof(uint32);
}

char *memdump_h(char *data)
{
	printf("%d", *(uint16 *)data);
	return data + sizeof(uint16);
}

char *memdump_p(char *data)
{
	printf("%lx", *(uint64 *)data);
	return data + sizeof(uint64);
}

char *memdump_S(char *data)
{
	printf("%s", data);
	return data + strlen(data) + 1;
}

char *memdump_s(char *data)
{
	uint64 *ptr = (uint64 *)data;
	printf("%s", (char *)*ptr);
	return data + sizeof(uint64);
}