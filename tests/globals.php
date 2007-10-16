<?

$T = new Blitz();
$T->load('test test test ');

$T->setGlobals(array('a' => '1234'));
var_dump($T->getGlobals());

?>
