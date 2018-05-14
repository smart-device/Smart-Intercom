<?php
$uploaddir = "pics/";
$uploadfile=$_GET['n'];

if (is_uploaded_file($_FILES['photo']['tmp_name'])) {
    if ($_FILES["photo"]["size"] > 500000) {
        echo " File is too large."; 
    } else {
        move_uploaded_file($_FILES['photo']['tmp_name'], $uploaddir.$uploadfile);
        $ourFileHandle = fopen("cntr.txt", 'r') or die("can't open file");
        $theData = fread($ourFileHandle , 1);
        fclose($ourFileHandle);
        if($theData<9)  $theData++;
        else            $theData=1;
        $ourFileHandle = fopen("cntr.txt", 'w') or die("can't open file");
        fwrite($ourFileHandle, $theData);
        fclose($ourFileHandle);
        copy($uploaddir.$uploadfile,$uploaddir.$theData.".jpg");
        echo " UpOK";
    } 
} else {
    echo " erRor"; 
}
?>
