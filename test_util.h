#define PREFIX_START (-24)
/* Smallest power of then for which there is a prefix defined.
   If the set of prefixes will be extended, change this constant
   and update the table "prefix". */

#include <stdio.h>
#include <math.h>

static char *eng(double value, int digits, int numeric)
{
  static char *prefix[] = {
  "y", "z", "a", "f", "p", "n", "u", "m", "",
  "k", "M", "G", "T", "P", "E", "Z", "Y"
  };
#define PREFIX_END (PREFIX_START+\
(int)((sizeof(prefix)/sizeof(char *)-1)*3))

      int expof10;
      static char result[100];
      char *res = result;

      if (value < 0.)
        {
            *res++ = '-';
            value = -value;
        }

      expof10 = (int) log10(value);
      if(expof10 > 0)
        expof10 = (expof10/3)*3;
      else
        expof10 = (-expof10+3)/3*(-3); 

      value *= pow(10,-expof10);

      if (value >= 1000.)
         { value /= 1000.0; expof10 += 3; }
      else if(value >= 100.0)
         digits -= 2;
      else if(value >= 10.0)
         digits -= 1;

      if(numeric || (expof10 < PREFIX_START) ||    
                    (expof10 > PREFIX_END))
        sprintf(res, "%.*fe%d", digits-1, value, expof10); 
      else
        sprintf(res, "%.*f %s", digits-1, value, 
          prefix[(expof10-PREFIX_START)/3]);
      return result;
}
