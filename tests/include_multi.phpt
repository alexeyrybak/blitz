--TEST--
multiple include cache test
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load(
'{{ BEGIN test1 }}test1:{{ include("include_multi.tpl"); }}{{ END }}'.
'{{ BEGIN test2 }}test2:{{ include("include_multi.tpl"); }}{{ END }}'); 

for ($i=0; $i<5; $i++) {
    $T->block('test1', array('var' => 'test1::'.$i));
    $T->block('test2', array('var' => 'test2::'.$i));
}

echo $T->parse();

?>

--EXPECT--
test1:var=test1::0
test1:var=test1::1
test1:var=test1::2
test1:var=test1::3
test1:var=test1::4
test2:var=test2::0
test2:var=test2::1
test2:var=test2::2
test2:var=test2::3
test2:var=test2::4
