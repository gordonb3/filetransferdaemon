#include <libeutils/ECGI.h>
#include <stdlib.h>

#include <tut/tut.hpp>

namespace tut
{
   using namespace EUtils;

   struct cgi {
   };

   typedef test_group<cgi> factory;
   typedef factory::object object;
}
namespace {
    tut::factory tf("ECGI test");
}

namespace tut {

   template<> template<> void object::test<1>() {
       set_test_name("Single cookie");

       setenv( "HTTP_COOKIE", "foo=foo1", 1 );
       ECGI cgi;
       cgi.parseCookies();
       ensure_equals( cgi.cookie("foo"), "foo1" );
   }

#if 0
   template<> template<> void object::test<2>() {
       set_test_name("Multiple cookies");

       setenv( "HTTP_COOKIE", "foo=foo1;bar=bar2;baz=baz3", 1 );
       ECGI cgi;
       cgi.parseCookies();
       ensure_equals( "First cookie", cgi.cookie("foo"), "foo1" );
       ensure_equals( "Second cookie", cgi.cookie("bar"), "bar2" );
       ensure_equals( "Third cookie", cgi.cookie("baz"), "baz3" );
   }
   template<> template<> void object::test<3>() {
       set_test_name("Non-value cookie");

       setenv( "HTTP_COOKIE", "foo", 1 );
       ECGI cgi;
       cgi.parseCookies();
       ensure( "Existing cookie", cgi.cookie("foo") == "foo" );
       ensure_not( "Non-existing cookie", cgi.cookie("bar") == "bar" );
   }
#endif
};
