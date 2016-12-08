# qMDB
uniquestudio quest 3

## something just to remind myself

request structure:

[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

first byte:instruction id((program control)sock_close=0 set=1 del=2 exist=3 get=4 register=5 unregister=6 sync=7 multi=8 execute=9)

if need loop following(for set command param)

{

2~5 byte:how long the params is

6+ bytes:param itself

}

reply structure:

[ ][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

first byte: status id.(normally 80 means SUCCESS ; 198 means INVALID KEY ; 200 means ILLEGAL OPERATION)

127 : OPERATOR & OPERAND MISMATCH ( e.g. provided non-address value for REGISTER )
 40 : REQUESTED SERVER UNREACHABLE 

2-5 byte : how long the reply string is

6+ byte : reply itself

use this protocol to ensure the binary safety
