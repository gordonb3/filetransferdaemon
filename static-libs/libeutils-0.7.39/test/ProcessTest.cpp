#include <libeutils/Process.h>
#include <libeutils/StringTools.h>
#include <iostream>
#include <string>
#include <algorithm>

#include <tut/tut.hpp>

namespace tut
{
   using namespace EUtils;

   void TrimSpaces( std::string& str)
   {
       size_t startpos = str.find_first_not_of(" \t"); // Find the first character position after excluding leading blank spaces
       size_t endpos = str.find_last_not_of(" \t"); // Find the first character position from reverse af
       if(( std::string::npos == startpos ) || ( std::string::npos == endpos))
       {
           str = "";
       }
       else
           str = str.substr( startpos, endpos-startpos+1 );

   }
   struct process {
   };

   typedef test_group<process> factory;
   typedef factory::object object;
}

namespace {
    tut::factory tf("Eutils::Process");
}

namespace tut {

   template<> template<> void object::test<1>() {
       set_test_name("echo foo want foo");

       Process p;
       std::string got;
       const char* cmd[] = { "echo", "foo", NULL };
       p.call(cmd);

       getline(*p.pout, got);
       TrimSpaces( got );
       ensure_equals( "echo foo gave foo", got, "foo" );
   }
   template<> template<> void object::test<2>() {
       set_test_name("echo foobar want foo");


       Process p;
       std::string got;
       const char* cmd[] = { "echo", "foobar", NULL };
       p.call(cmd);

       getline(*p.pout, got);
       TrimSpaces( got );
       ensure_not( "echo foobar didn't give foo", got == "foo" );
   }

   template<> template<> void object::test<3>() {
       set_test_name("bad pipe to cat");

       Process p;
       std::string got;
       *p.pin << "hello";
       const char* cmd[] = { "cat", NULL };
       p.call(cmd);
       getline(*p.pout, got);
       TrimSpaces( got );
       ensure_equals( "got hello", got,  "hello" );
       ensure_not( "didn't get hello world", got == "hello world" );

   }
   template<> template<> void object::test<4>() {
       set_test_name("two lines");

       Process p;
       std::string line1("hello");
       std::string line2("world");
       std::string got;
       std::string wanted(line1 + "\n" + line2);
       const char* cmd[] = { "echo", "-e", const_cast<char*>(wanted.c_str()) , NULL };
       p.call(cmd);
       getline(*p.pout, got);
       TrimSpaces( got );
       ensure_equals( "line 1", got, line1 );
       getline(*p.pout, got);
       TrimSpaces( got );
       ensure_equals( "line 2", got, line2 );

   }

   template<> template<> void object::test<5>() {
       set_test_name("perl script with eval");

       Process p;
       std::string got;
       *p.pin << "1 + 1";
       const char* cmd[] = { "perl", "-pe", "$_ = eval $_", NULL };
       p.call(cmd);
       getline(*p.pout, got);
       TrimSpaces( got );
       ensure_equals( "evaluation correct", got, "2" );

   }

   template<> template<> void object::test<6>() {
       set_test_name("writing to STDERR");

       Process p;
       std::string got;
       std::string wanted("foo42");
       const char* cmd[] = { "perl", "-e", "print STDERR \"foo42\"", NULL };
       p.call(cmd);
       getline(*p.perr, got);
       TrimSpaces( got );
       ensure_equals( wanted, got );

   }
   template<> template<> void object::test<7>() {
       set_test_name("false");

       Process p;
       const char* cmd[] = { "false", NULL };
       ensure_equals( "program returned true", p.call( cmd ), 1 );
   }

   template<> template<> void object::test<8>() {
       set_test_name("true");

       Process p;
       const char* cmd[] = { "true", NULL };
       ensure_equals( "program returned false", p.call( cmd ), 0 );
   }
};
