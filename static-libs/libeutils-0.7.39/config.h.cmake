#ifndef CONFIG_H
#define CONFIG_H

/* Defined to current used TCL version */
#cmakedefine TCL_VERSION @TCL_VERSION@

/* Defined if we have tcl expect */
#cmakedefine HAVE_EXPECT_H

/* Defined if we have popt */
#cmakedefine HAVE_POPT_H

/* Defined if we have pthreads */
#cmakedefine CMAKE_HAVE_PTHREAD_H

#endif // CONFIG_H vim:ft=cpp

