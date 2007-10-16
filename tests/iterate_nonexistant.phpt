--TEST--
nonexistant path iteration
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('{{ BEGIN main }} main parsed {{ BEGIN first }} first {{ END }} {{ BEGIN second }} second {{ END }} {{ END }}');

$T->iterate('/main');
$T->context('/main/first');
$T->block('5588/lklk');
$T->iterate('5588/lklk');

echo $T->parse();

?>
--EXPECT--
main parsed
