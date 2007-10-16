<?

$T = new Blitz('ex8A.tpl');

$max_num_list = 5;

// use context & iterate
$T->context('row');
for($i=0; $i<$max_num_list; $i++) {
    $T->iterate();
    $T->set(array('i' => $i));
}

// or just use block
for($i=0; $i<$max_num_list; $i++) {
    $T->block('/row',array('i' => $i));
}

echo $T->parse();

?>

