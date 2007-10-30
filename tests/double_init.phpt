--TEST--
double Blitz object initialization
--FILE--
<?php

$t=new Blitz(); 
$t->load("{{Blitz()}}"); 
$t->parse();

echo "Done\n";
?>
--EXPECTF--	
Warning: blitz::blitz(): The object has already been initialized in %s on line %d
Done
