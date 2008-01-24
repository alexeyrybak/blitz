--TEST--
Bug #83 (double free on include())
--FILE--
<?php

include "common.inc";

$s=<<<E
 >{{ include('test.tpl') }}<
E;
$t=new Blitz;
$t->load($s);
var_dump($t->parse());

echo "Done\n";
?>
--EXPECT--	
string(26) " >this template is empty
<"
Done
