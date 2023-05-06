/*!
  @file unittest_tfile.c
  @brief These code implementations manage the process of test files by collecting their creation dates.
  By comparing the dates, the code can determine if a test file needs to be updated, and then proceed to
  compile the update accordingly.

  @author Erick Carrillo.
  @copyright Copyright (C) 2022, Erick Alejandro Carrillo López, All rights reserved.
  @license This project is released under the MIT License
*/

#include "../include/unittest_tfile.h"

#include "../include/unittest_map.h"

#include <assert.h> /* assert() */
#include <dirent.h> /* opendir(), closedir() */
#include <except.h> /* throw_except() */
#include <fcntl.h>  /* create() */
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> /* stat() */
#include <time.h>     /* ctime() */
#include <unistd.h>   /* access() */
#define F TestFile

F      *head_files	    = NULL;
uint8_t dumped		    = 0;
size_t	amount_hashed_dates = 0;
long	hashed_dates[MAX_AMOUNT_OF_FILES];
size_t	new_amount_hashed_dates = 0;
long	new_hashed_dates[MAX_AMOUNT_OF_FILES];

/* TODO: Rewrite this exceptions */
Except UnittestErrorCreatingFile = {"Error creating a new file at \"TEST_DIR\""};
Except UnittestErrorOpeningFile	 = {"Error opening a file at \"TEST_DIR\""};
Except UnittestErrorReadingFile	 = {"Error reading a file at \"TEST_DIR\""};
Except UnittestErrorWrittingFile = {"Error writting a file at \"TEST_DIR\""};
Except UnittestNotEnoughMemory	 = {"Not enough memory on the heap"};
Except UnittestErrorTestBaseDoesntExist = {"Error the default \"TEST_DIR\" doesn't exist"};

/* put_new_dates: Puts new creation/modification dates of the test files in the file. */
void put_new_dates(const char *test_dir, const char *filename)
{
	FILE	    *fp;
	const size_t len = strlen(test_dir) + strlen(filename);
	char	     path[len];

	memset(path, 0, len);
	strcat(path, test_dir);
	strcat(path, filename);

	if (access(path, F_OK) == -1) /* Doens't exist the file */
		throw_except(UnittestErrorReadingFile);

	if ((fp = fopen(path, "wb")) == NULL) throw_except(UnittestErrorOpeningFile);

	/* Read the hashed binaries*/
	if (fwrite(&new_amount_hashed_dates, sizeof(size_t), 1, fp) != 1)
		throw_except(UnittestErrorWrittingFile);

	if (fwrite(new_hashed_dates, sizeof(long), new_amount_hashed_dates, fp) !=
	    new_amount_hashed_dates)
		throw_except(UnittestErrorWrittingFile);
	fclose(fp);
}

/* get_prev_dates: Gets the previous modification dates of a file in a specific directory.
 */
void get_prev_dates(const char *test_dir, const char *filename)
{
	FILE	    *fp;
	const size_t len = strlen(test_dir) + strlen(filename);

	if (dumped != 0) return; /* Finish the function */
	dumped = 1;

	char path[len];

	memset(path, 0, len);
	strcat(path, test_dir);
	strcat(path, filename);

	if (access(path, F_OK) == -1) { /* Doens't exist the file */
		if (creat(path, 0664) == -1) throw_except(UnittestErrorCreatingFile);
		return;
	}

	if ((fp = fopen(path, "rb")) == NULL) throw_except(UnittestErrorOpeningFile);

	/* Read the hashed binaries*/
	if (fread(&amount_hashed_dates, sizeof(size_t), 1, fp) != 1)
		throw_except(UnittestErrorReadingFile);

	if (fread(hashed_dates, sizeof(long), amount_hashed_dates, fp) !=
	    amount_hashed_dates)
		throw_except(UnittestErrorReadingFile);

	fclose(fp);
}

/* get_creation_date: This function gets the creation time of a file located at a given
   path
   and stores it as a string in the date parameter. */
void get_creation_date(const char *path_file, char *date)
{
	struct stat attr;

	assert(date != NULL && path_file != NULL && "You can't pass null arguemnt");

	/* TODO: Check if the file exist */

	stat(path_file, &attr);
	sprintf(date, "Last modified time: %s", ctime(&attr.st_mtime));
}

/* needs_update: checks if a file needs to be updated based on its hashed creation date.
 */
uint8_t needs_update(long date_hashed)
{
	size_t i;

	for (i = 0; i < amount_hashed_dates; i++)
		if (date_hashed ==
		    hashed_dates[i]) { /* If there weren't chages return 0 */
			hashed_dates[i] = hashed_dates[amount_hashed_dates - 1];
			amount_hashed_dates--;
			return 0;
		}
	return 1;
}

/* include: This function includes a file with a given filename into a specific test
 * directory. */
void include(const char *test_dir, const char *filename)
{
	assert(filename != NULL && test_dir != NULL && "Erro can't be null");

	/* First check if the directoy exist */
	DIR *dir;
	if ((dir = opendir(test_dir)) == NULL) {
		closedir(dir);
		throw_except(UnittestErrorTestBaseDoesntExist);
	}
	closedir(dir);

	long h;
	char date[50], path[255];
	F   *new_file;

	/* Map the name of the filename */
	const uint8_t *ptr =
		unittest_map_find((const uint8_t *) filename, strlen(filename));
	if (ptr != NULL) /* It is already include it */
		return;

	memset(path, 0, 255);
	strcat(path, test_dir);
	strcat(path, filename);

	get_creation_date(path, date);
	h = unittest_hash((const uint8_t *) date);

	if ((new_file = malloc(sizeof(F))) == NULL) throw_except(UnittestNotEnoughMemory);

	new_file->date_hashed = h;
	/* Map the file name */
	new_file->filename =
		(char *) unittest_map((const uint8_t *) filename, strlen(filename));
	new_file->next = head_files;
	head_files     = new_file;

	/* Append the new hashed dates */
	new_hashed_dates[new_amount_hashed_dates++] = h;
}

#undef F
