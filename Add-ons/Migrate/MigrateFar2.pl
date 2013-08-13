#~/bin/perl -w
use Win32::Registry;

print "DELETE FROM `table_keys` WHERE `id`>1;\n";
print "DELETE FROM `table_values` WHERE `key_id`>1;\n";

my $key = 2;

MigratePresets('EditFilterPresets');
MigratePresets('EditFindPresets');
MigratePresets('EditReplacePresets');
MigratePresets('EditTransliteratePresets');
MigratePresets('FileAdvancedPresets');
MigratePresets('FileFindPresets');
MigratePresets('FileGrepPresets');
MigratePresets('FileQuickRenamePresets');
MigratePresets('FileRenamePresets');
MigratePresets('FileReplacePresets');
MigratePresets('ViewFindPresets');

MigrateBatches('EditorBatches');
MigrateBatches('PanelBatches');

sub CreateKey
{
	my ($root, $name) = @_;

	print "INSERT INTO `table_keys` (`id`, `parent_id`, `name`) VALUES ($key, $root, '$name');\n";
	return $key++;
}

sub CreateValue
{
	my ($root, $name, $value) = @_;

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

sub MigratePresets
{
	my ($name) = @_;

	RecursiveCopy(1, $name);
}

sub MigrateBatches
{
	my ($name) = @_;

	RecursiveCopy(1, $name);
}
