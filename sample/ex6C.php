<?

class BlitzTemplate extends Blitz {
    var $titem;

    function BlitzTemplate($t,$titem) {
        parent::Blitz($t);
        $this->set(array('parent_val' => 'some_parent_val'));
        $this->titem = $titem;
    }

    function my_test() {
        return 'user method called ('.__CLASS__.','.__LINE__.')';
    }

    function test_include() {
        $result = '';
        $i = 0;
        while($i++<3) {
            $result .= $this->include($this->titem,array(
                'child_val' => 'i_'.$i
            ));
        }
        return $result;
    }

}

$T = new BlitzTemplate('ex6A.tpl','ex6B.tpl');
echo $T->parse();

?>
