<?

class BlitzTemplate extends Blitz {
    function BlitzTemplate($t) {
        parent::Blitz($t);
        $this->set(array('x' => 2006));
    }

    function my_test($p1,$p2,$p3,$p4) {
        $result = 'user method called ('.__CLASS__.','.__LINE__.')'."\n";
        $result .= 'parameters are:'."\n";
        $result .= '1:'.var_export($p1,TRUE)."\n";
        $result .= '2:'.var_export($p2,TRUE)."\n";
        $result .= '3:'.var_export($p3,TRUE)."\n";
        $result .= '4:'.var_export($p4,TRUE)."\n";
        return $result;
    }
}

$T = new BlitzTemplate('ex7A.tpl');
echo $T->parse();

?>
