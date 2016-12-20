#!/usr/bin/perl

use FindBin;
use Encode;
use Encode::CN;
#use Encode::Detect::Detector;

#print "$FindBin::Bin\n";
#print "$FindBin::Script\n"; 
 
my $path = ".";

opendir(DIR, "$path") || die "Cannot open dir : $!";
my @files = readdir(DIR);
foreach my $file (@files) {
   if(-f "$path/$file") {
        #print $file."\n";
        
        #open(FD,"$file")||die("Can not open the file!$!n");
        #$line=<FD>;
        #$line=encode("gbk",decode("utf-8",$line)) unless (detect($line) =~ /gb/);
        #print $line;
        #$line = encode("gbk", decode("utf8", $line));
        #print FD "@line";
        #close(FD);
        
        #system("perl f://Markdown.pl $file > $file.html");
        
        $pos = index($file, ".markdown");
        if($pos > 0) {
            #print $pos."\n";
            $name = substr($file, 0, $pos);
            #print $name."\n";
            system("perl f://Markdown.pl $file > $name.html");
        }
    }
}
closedir(DIR);