--TEST--
contexts
--FILE--
<?php
include('common.inc');
$T = new Blitz('context.tpl');

$max_num_list = 5;
$max_num_item = 5;

for($i=0; $i<$max_num_list; $i++) {
    $T->context('/list');
    $T->block('',array('list_num' => $i));
    $is_empty = $i%2; // emulate empty sub-lists

    if($is_empty) {
        $T->block('list_empty');
    } else {
        for($j=0; $j<$max_num_item; $j++) {
           $T->block('list_item',array('i_row' => $i.':'.$j));
        }
    }
}

echo $T->parse();
?>
--EXPECT--
complex list example
list #0
 row #0:0
 row #0:1
 row #0:2
 row #0:3
 row #0:4

list #1
 this list is empty

list #2
 row #2:0
 row #2:1
 row #2:2
 row #2:3
 row #2:4

list #3
 this list is empty

list #4
 row #4:0
 row #4:1
 row #4:2
 row #4:3
 row #4:4
