# qMDB
uniquestudio quest 3

## something just to remind myself

request structure:

[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

first byte:instruction id(set=1 del=2 exist=3 get=4 register=5 unregister=6 sync=7)
if need loop following(for set command param)
{
2~5 byte:how long the params is
6+ bytes:param itself
}

use this protocol to ensure the binary safety
