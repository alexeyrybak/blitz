<?

class BlitzTemplate extends Blitz {

    function BlitzTemplate($t) {
        parent::Blitz($t);
    }

    function my_test() {
        return 'user method called ('.__CLASS__.",".__LINE__.')';
    }
}

$T = new BlitzTemplate('ex5A.tpl');
echo $T->parse();

?>
