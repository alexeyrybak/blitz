--TEST--
include with context iteration
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('{{ include("include_ctx.tpl") }}');

$i = 0;
while ($i<5) {
    $T->block('/test1', array('var' => $i), TRUE);
    $T->block('/test2', array('var' => $i), TRUE);
    $i++;
}

echo $T->parse();

?>
--EXPECT--
test1:  v1=0  v1=1  v1=2  v1=3  v1=4 
test2:  v2=0  v2=1  v2=2  v2=3  v2=4

