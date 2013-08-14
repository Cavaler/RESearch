#!/bin/perl -w
use Win32::Registry;

print "DELETE FROM `table_keys` WHERE `id`>1;\n";
print "DELETE FROM `table_values` WHERE `key_id`>=1;\n";

my $key = 1;
RecursiveCopy(0, '', '');

sub CreateKey
{
	my ($root, $name) = @_;

	if ($root > 0) {
		print "INSERT INTO `table_keys` (`id`, `parent_id`, `name`) VALUES ($key, $root, '$name');\n";
	}
	return $key++;
}

sub CreateValue
{
	my ($root, $name, $value) = @_;

	return if $name eq 'AllCP';

	$value =~ s/'/''/g;

	print "INSERT INTO `table_values` (`key_id`, `name`, `value`) VALUES ($root, '$name', '$value');\n";
}

sub RecursiveCopy
{
	my ($root, $name, $sub) = @_;

	my $tkey = CreateKey($root, $name);

	my $hkey;
	$::HKEY_CURRENT_USER->Open("Software\\Far2\\Plugins\\RESearch\\".$sub.$name, $hkey);

	my %rvalues;
	$hkey->GetValues(\%rvalues);

	foreach $rv (keys %rvalues) {
		CreateValue($tkey, $rv, $rvalues{$rv}[2]);
	}

	my @rkeys;
	$hkey->GetKeys(\@rkeys);

	foreach $rkey (@rkeys) {
		RecursiveCopy($tkey, $rkey, $sub.$name."\\");
	}
}
