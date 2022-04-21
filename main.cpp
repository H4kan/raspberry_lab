#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#ifndef	INPUT
#define	INPUT "input"
#define OUTPUT "output"
#endif

float getTime(timeval* start, timeval* end)
{
	return (end->tv_sec - start->tv_sec);
}

int main(int argc, char **argv)
{
	char *chipname = "gpiochip0";

	struct gpiod_line_bulk lines;
	struct gpiod_line_bulk events;

	unsigned int offsets[2];
	int values[2];

	unsigned int led1_num = 23;
	unsigned int led2_num = 27;
	unsigned int button1_num = 17;
	unsigned int button2_num = 18;

	offsets[0] = button1_num;
	offsets[1] = button2_num;
	values[0] = -1;
	values[1] = -1;
	
	unsigned int val;
	struct gpiod_chip *chip;
	struct gpiod_line *led1, *led2;
	int i, ret;
	unsigned int start = 1;
	int buttonVal;
	timeval startClock, endClock, baseClock;
	unsigned int buttonPressed = 0;
	

	chip = gpiod_chip_open_by_name(chipname);
	if (!chip) {
		perror("Open chip failed\n");
		goto end;
	}

	ret = gpiod_chip_get_lines(chip, offsets, 2, &lines);
	if(ret)
	{
		perror("gpiod_chip_get_lines");
		goto close_chip;
	}

	led1 = gpiod_chip_get_line(chip, led1_num);
	if (!led1) {
		perror("Get line failed\n");
		goto close_chip;
	}
	ret = gpiod_line_request_output(led1, INPUT, 0);
	if (ret < 0) {
		perror("Request line as output failed\n");
		goto release_line;
	}

	led2 = gpiod_chip_get_line(chip, led2_num);
	if (!led2) {
		perror("Get line failed\n");
		goto close_chip;
	}
	ret = gpiod_line_request_output(led2, INPUT, 0);
	if (ret < 0) {
		perror("Request line as output failed\n");
		goto release_line;
	}

	struct timespec ts;
	ts.tv_sec = 0;
    ts.tv_nsec = 30 * 1000000;

	 ret = gpiod_line_request_bulk_falling_edge_events(&lines, "falling edge example");
	if(ret)
	{
		perror("gpiod_line_request_bulk_rising_edge_events");
		goto release_line;
	}
	
	while (true)
	{
		ret = gpiod_line_event_wait_bulk(&lines, NULL, &events);
		if(ret == -1)
		{
			perror("gpiod_line_event_wait_bulk");
			goto release_line;
		}
		else if(ret == 0)
		{
			fprintf(stderr, "wait timed out\n");
			goto release_line;
		}
		nanosleep(&ts, NULL);
		ret = gpiod_line_get_value_bulk(&events, values);
		if(ret)
		{
			perror("gpiod_line_get_value_bulk");
			goto release_line;
		}
	
		if (!buttonPressed && values[0] == 0)
		{
			gpiod_line_set_value(led1, 1);
				if (start > 0)
				{	
					/* start measuring */
					gettimeofday(&baseClock, NULL);
					gettimeofday(&startClock, NULL);
					start = 0;
				}
				else
				{
					/* end measure, start measure */
					gettimeofday(&endClock, NULL);
					printf("%f seconds\n", getTime(&startClock, &endClock) );
					fflush(stdout);
					startClock = endClock;
				}
				buttonPressed = 1;
				continue;
		}
		else if (buttonPressed && values[0] != 0)
		{
			gpiod_line_set_value(led1, 0);
			buttonPressed = 0;
		}
		if (values[1] == 0)
		{
			gpiod_line_set_value(led2, 1);
			if (start == 0)
			{
				/* end measure */
				gettimeofday(&endClock, NULL);
				printf("Total time %f seconds\n", getTime(&baseClock, &endClock));
				fflush(stdout);
			}
			break;
		}
	}

	sleep(1);
	gpiod_line_set_value(led2, 0);

release_line:
	gpiod_line_release_bulk(&lines);
	gpiod_line_release_bulk(&events);
	gpiod_line_release(led1);
	gpiod_line_release(led2);
close_chip:
	gpiod_chip_close(chip);
end:
	return 0;
}
