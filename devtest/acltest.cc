#include <sys/types.h>
#include <sys/acl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
  FILE* f = fopen("tst.dat","w");
  if (!f) {
    perror("tst.dat");
    return -1;
  }

#if 1
  const char* sacl = "user::rw-\nuser:psdatmgr:rwx\ngroup::r--\nmask::rwx\nother::r--\n";
  printf("%s\n",sacl);

  acl_t acl_out = acl_from_text(sacl);
  if ( acl_valid(acl_out) )
    perror("acl_out");

  if ( (acl_set_fd(fileno(f),acl_out)) )
    perror("acl_set_fd");

  acl_free((void*)acl_out);
#else
  { acl_t acl_in = acl_get_fd(fileno(f));
    int   lacl;
    char* sacl = acl_to_text(acl_in,&lacl);

    printf("acl %s[%d]:\n%s\n",
           "tst.dat",lacl,sacl);

    const char* ACL_ADD = "user:psdatmgr:rwx\nmask::rxw\n";
    char* sacl_out = new char[lacl+strlen(ACL_ADD)+1];
    sprintf(sacl_out,"%s%s",ACL_ADD,sacl);

    printf("%s",sacl_out);

    acl_t acl_out = acl_from_text(sacl_out);
    printf("%s",acl_to_text(acl_out,0));

    if ( acl_valid(acl_out) )
      perror("acl_out");

    if ( (acl_set_fd(fileno(f),acl_out)) ) {
      perror("acl_set_fd");
    }

    delete[] sacl_out;
    acl_free((void*)acl_out);
    acl_free(sacl);
    acl_free((void*)acl_in);
  }
#endif
  { acl_t acl_in = acl_get_fd(fileno(f));
    ssize_t lacl;
    char* sacl = acl_to_text(acl_in,&lacl);
    printf("acl %s[%d]:\n%s\n",
           "tst.dat",lacl,sacl);
    acl_free(sacl);
    acl_free((void*)acl_in);
  }

  return 1;
}
