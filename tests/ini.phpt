--TEST--
ini-values settings test
--FILE--
<?php
include('common.inc');

ini_set('blitz.var_prefix', '@');
ini_set('blitz.tag_open',  '<?blitz');
ini_set('blitz.tag_close','?>');

$T = new Blitz();
$T->load("<?blitz @name ?>\n");

$T->set(array('name' => 'name_val'));

echo $T->parse();

?>
--EXPECT--
name_val
