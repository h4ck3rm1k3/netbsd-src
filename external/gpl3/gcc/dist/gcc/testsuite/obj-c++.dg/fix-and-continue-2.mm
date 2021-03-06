/* Static variables, even if local, require indirect access through a stub
   if -mfix-and-continue is enabled.  */

/* Author: Ziemowit Laski <zlaski@apple.com> */
   
/* { dg-do assemble { target *-*-darwin* } } */
/* { dg-options "-mfix-and-continue" } */

#include "../objc-obj-c++-shared/Object1.h"

@interface Foo: Object
+ (Object *)indexableFileTypes;
@end

@implementation Foo
+ (Object *)indexableFileTypes
{
  static Object *fileTypes = 0;
  if(!fileTypes) {
    fileTypes = [Object new];
  }
  return fileTypes;
}
@end
