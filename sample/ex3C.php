<?

$T = new Blitz('ex3A.tpl');
$T->set_global(
    array(
        'a' => 'a_value', 
        'b' => 'b_value'
    )
);

echo $T->parse();
echo "\n";

?>
