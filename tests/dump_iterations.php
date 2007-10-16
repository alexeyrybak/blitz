<?

$T = new Blitz('dump_iterations.tpl');
for ($i = 0; $i < 3; $i++)
{
    $T->block('context', array('i' => $i));
}
$T->dumpIterations();

?>
