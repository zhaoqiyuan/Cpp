#include <stdio.h>

void LibTestTest()
{
	int count = 0;
	for (int i = 0; i < 1000000; ++i)
	{
		for (int j = 0; j < 10000; ++j)
		{
			count += j;
		}
	}
	printf("LibTest %d\n", count);
	return;
}
