<?

$T = new Blitz('ex10A.tpl');

// online
echo $T->fetch('online')."\n";

// away
echo $T->fetch('away')."\n";

$T->context('offline');

// 15 days ago
$T->iterate('d');
echo $T->fetch('offline', array('n' => 15))."\n";

$T->iterate(); // create next iteration for offline block

// 2 months ago
$T->iterate('m');
echo $T->fetch('offline', array('n' => 2))."\n";

?>
