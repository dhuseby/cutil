/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#include <stdint.h>
#include <stdlib.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/sanitize.h>

static int added_tz = FALSE;

static void test_sanitize_environment( void )
{
	int i = 0;
	int8_t ** new_env = NULL;

	new_env = build_clean_environ( 0, (int8_t**)NULL, 0, (int8_t**)NULL );

	CU_ASSERT_EQUAL( strcmp( new_env[0], "IFS= \t\n" ), 0 );
	CU_ASSERT_EQUAL( strcmp( new_env[1], "PATH=" _PATH_STDPATH ), 0 );
}

static int8_t * preserve_environ[] =
{
	"USER",
	NULL
};

static void test_sanitize_environment_preserve( void )
{
	int i = 0;
	int8_t ** new_env = NULL;

	new_env = build_clean_environ( 1, preserve_environ, 0, (int8_t**)NULL );

	CU_ASSERT_EQUAL( strcmp( new_env[0], "IFS= \t\n" ), 0 );
	CU_ASSERT_EQUAL( strcmp( new_env[1], "PATH=" _PATH_STDPATH ), 0 );
	CU_ASSERT_EQUAL( strncmp( new_env[2], "TZ=", 3 ), 0 );
	CU_ASSERT_EQUAL( strncmp( new_env[3], "USER=", 5 ), 0 );
}

static int8_t * add_environ[] =
{
	"FOO=BAR",
	NULL
};

static void test_sanitize_environment_add( void )
{
	int i = 0;
	int8_t ** new_env = NULL;

	new_env = build_clean_environ( 0, (int8_t**)NULL, 1, add_environ );

	CU_ASSERT_EQUAL( strcmp( new_env[0], "IFS= \t\n" ), 0 );
	CU_ASSERT_EQUAL( strcmp( new_env[1], "PATH=" _PATH_STDPATH ), 0 );
	CU_ASSERT_EQUAL( strncmp( new_env[2], "TZ=", 3 ), 0 );
	CU_ASSERT_EQUAL( strcmp( new_env[3], "FOO=BAR" ), 0 );
}

static void test_sanitize_open_files( void )
{
	struct stat st;
	int8_t tname[32];
	int fd = -1;

	MEMSET( tname, 0, 32 );
	strcpy( tname, "blahXXXXXX" );
	
	fd = mkstemp( tname );

	/* make sure the temp file is open */
	CU_ASSERT_NOT_EQUAL( fd, -1 );
	CU_ASSERT_NOT_EQUAL( fstat( fd, &st ), -1 );

	sanitize_files( NULL, 0 );

	/* make sure the temp file is closed */
	CU_ASSERT_EQUAL( fstat( fd, &st ), -1 );
	CU_ASSERT_EQUAL( errno, EBADF );

	/* clean up after ourselves */
	unlink( tname );
}

static void test_sanitize_closed_std_descriptors( void )
{
	struct stat st;

	/* close stdin */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	sanitize_files( NULL, 0 );

	/* make sure stdin, stdout, stderr were re-opened */
	CU_ASSERT_NOT_EQUAL( fstat( STDIN_FILENO, &st ), -1 );
	CU_ASSERT_NOT_EQUAL( fstat( STDOUT_FILENO, &st ), -1 );
	CU_ASSERT_NOT_EQUAL( fstat( STDERR_FILENO, &st ), -1 );
}


static int init_sanitize_suite( void )
{
	srand(0xDEADBEEF);

	/* set the TZ variable if it doesn't exist */
	if ( getenv( "TZ" ) == NULL )
	{
		putenv( "TZ=GST+8" );
		added_tz = TRUE;
	}

	return 0;
}

static int deinit_sanitize_suite( void )
{
	if ( added_tz )
	{
		unsetenv( "TZ" );
	}
	return 0;
}

static CU_pSuite add_sanitize_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "sanitize environment to empty test", test_sanitize_environment), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "sanitize environment w/preserve test", test_sanitize_environment_preserve), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "sanitize environment w/add test", test_sanitize_environment_add), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "close open files sanitize files test", test_sanitize_open_files), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "open closed std file descriptors sanitize files test", test_sanitize_closed_std_descriptors), NULL );
	return pSuite;
}

CU_pSuite add_sanitize_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Sanitize Tests", init_sanitize_suite, deinit_sanitize_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in sanitize specific tests */
	CHECK_PTR_RET( add_sanitize_tests( pSuite ), NULL );

	return pSuite;
}

