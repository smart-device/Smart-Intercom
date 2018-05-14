<?php
    $input = $_GET['i'];
	if($input==1 || $input==2 || $input==3) {
        if($input!=3) {   
		    $ourFileHandle = fopen("key.txt", 'w') or die("can't open file");
		    fwrite($ourFileHandle, $input);
	        fclose($ourFileHandle);
	    }
		$ourFileHandle = fopen("rel.txt", 'r') or die("can't open file");
		$theData = fread($ourFileHandle , 1);
		fclose($ourFileHandle);
		echo("UUU");echo($theData);
	} else {
        $output = $_GET['o'];
        if($output==1 || $output==2 || $output==3){
    		$ourFileHandle = fopen("rel.txt", 'w') or die("can't open file");
    		fwrite($ourFileHandle, $output);
    		fclose($ourFileHandle);
        }
	}
	fclose($ourFileHandle);
?>
