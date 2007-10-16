<?

$T = new Blitz('ex9A.tpl');

$max_num_list = 5;
$max_num_item = 5;

$T->context('/list');
for($i=0; $i<$max_num_list; $i++) {
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

