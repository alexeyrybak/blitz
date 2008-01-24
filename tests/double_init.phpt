--TEST--
double Blitz object initialization
--FILE--
<?php

include('common.inc');

$T=new Blitz(); 
$T->load("{{Blitz()}}"); 
$T->parse();

echo "Done\n";
?>
--EXPECT--	
the object has already been initialized
Done
