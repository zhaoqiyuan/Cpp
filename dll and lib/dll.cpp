#include <stdio.h>

void _declspec(dllexport) DllTestTest()
{
	int count = 0;
	for (int i = 0; i < 1000000; ++i)
	{
		for (int j = 0; j < 10000; ++j)
		{
			count += j;
		}
	}
	printf("DllTest %d\n", count);
	return;
}
