<?

$T = new Blitz('ex1A.tpl');
$i = 0;
$i_max = 10;

for ($i = 0; $i<$i_max; $i++) {
    echo $T->parse(
        array(
            'a' => 'var_'.(2*$i),
            'b' => 'var_'.(2*$i+1),
            'i' => $i
        )
    );
}

?>
