# Usermods : IRA_NATSIO usermod

This usermod is base on the new version 2 usermods! See EXAMPLE_v2 for more info\
This is still work in progress.

## Installation 

Uncomment the following lines in `usermods_list.cpp` and compile!  

>#ifdef IRA_NATSIO\
>  #include "../usermods/IRA_NatsIO_client/IRA_NatsIO_client.h"\
>#endif
>
>...
>
>#ifdef IRA_NATSIO\
>  usermods.add(new IRA_NatsIO());\
>#endif

and define the IRA_NATSIO variable at compile time to include the usermod

