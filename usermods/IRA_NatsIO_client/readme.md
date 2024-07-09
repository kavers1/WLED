# Usermods API v2 example usermod

In this usermod file you can find the documentation on how to take advantage of the new version 2 usermods!

## Installation 

Copy `usermod_v2_example.h` to the wled00 directory.  
Uncomment the following lines in `usermods_list.cpp` and compile!  

#ifdef IRA_NATSIO
  #include "../usermods/IRA_NatsIO_client/IRA_NatsIO_client.h"
#endif
...
#ifdef IRA_NATSIO
  usermods.add(new IRA_NatsIO());
#endif

and define the IRA_NATSIO variable at compile time to include the usermod

