#include <stdio.h>
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <signal.h>


int memento()
{
	int a = 0;
	MessageBoxA(NULL, "Memento mori", "POSIX Signal", NULL);
	return 0;
}
void fall()
{
	int* p = 0x00000000;
	*p = 13;
}
void posix_death_signal(int signum)
{
	memento();
	signal(signum, SIG_DFL);
	exit(3);
}


int main(int argc, char *argv[])
{
	signal(SIGSEGV, posix_death_signal);
	fall();
}