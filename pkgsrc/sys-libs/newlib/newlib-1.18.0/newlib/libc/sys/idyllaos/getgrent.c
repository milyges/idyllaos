#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static struct group gr_group;	/* password structure */
static FILE * group_fp;

static char name[1024];
static char members[1024];

struct group * getgrgid(gid_t gid)
{
 FILE * fp;
 char buf[1024];

 if ((fp = fopen (_PATH_GROUP, "r")) == NULL)
 {
  return NULL;
 }
 while (fgets(buf, sizeof(buf), fp))
 {
  sscanf(buf, "%[^:]:%d:%s\n", name, &gr_group.gr_gid, members);
  gr_group.gr_name = name;
  // TODO: Group members
  if (gid == gr_group.gr_gid)
  {
   fclose(fp);
   return &gr_group;
  }
 }
 return NULL;
}

struct group * getgrnam(const char * grname)
{
 FILE * fp;
 char buf[1024];

 if ((fp = fopen (_PATH_GROUP, "r")) == NULL)
 {
  return NULL;
 }
 while (fgets(buf, sizeof(buf), fp))
 {
  sscanf(buf, "%[^:]:%d:%s\n", name, &gr_group.gr_gid, members);
  gr_group.gr_name = name;
  // TODO: Group members
  if (!strcmp(grname, gr_group.gr_name))
  {
   fclose(fp);
   return &gr_group;
  }
 }
 return NULL;
}

struct group * getgrent(void)
{
 char buf[1024];
 if (group_fp == NULL)
    return NULL;
 if (fgets(buf, sizeof(buf), group_fp) == NULL)
    return NULL;
 sscanf(buf, "%[^:]:%d:%s\n", name, &gr_group.gr_gid, members);
 gr_group.gr_name = name;
 // TODO: Group members
 return &gr_group;
}

void setgrent(void)
{
 if (group_fp != NULL)
    fclose(group_fp);

 group_fp = fopen (_PATH_GROUP, "r");
}

void endgrent(void)
{
 if (group_fp != NULL)
    fclose(group_fp);
 group_fp = NULL;
}
