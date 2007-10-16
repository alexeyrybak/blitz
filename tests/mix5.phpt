--TEST--
mix #5
--FILE--
<?php
include('common.inc');

$body = <<<BODY
Root:{{ \$start }}
{{BEGIN test }}Item:{{ \$item }}
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);
$T->iterate('/test');
$T->set(array('item' => 'item_value_1'));
$T->set(array('start' => 'start_1'));

$T->iterate('/');
$T->block('/test', array('item' => 'item_value_2'));
$T->block('/test', array('item' => 'item_value_3'));
$T->context('/');
$T->set(array('start' => 'start_2'));

echo $T->parse();

?>
--EXPECT--
Root:start_1
Item:
Root:start_2
Item:item_value_2
Item:item_value_3
