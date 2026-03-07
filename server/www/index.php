<?php
/**
 * Dystopia MUD — Public Status Page
 *
 * Reads config.json for display values. No authentication required.
 */

$config_file = dirname(__FILE__) . '/config.json';
$config = array(
    'mud_name'    => 'Dystopia MUD',
    'hostname'    => 'coming.soon',
    'port'        => 8000,
    'description' => 'A dark cyberpunk text-based multiplayer game.',
    'theme'       => 'cyberpunk',
);

if (file_exists($config_file)) {
    $json = file_get_contents($config_file);
    if ($json !== false) {
        $parsed = json_decode($json, true);
        if (is_array($parsed)) {
            $config = array_merge($config, $parsed);
        }
    }
}

$theme = preg_replace('/[^a-z]/', '', strtolower($config['theme']));
$name  = htmlspecialchars($config['mud_name']);
$host  = htmlspecialchars($config['hostname']);
$port  = (int) $config['port'];
$desc  = htmlspecialchars($config['description']);
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?php echo $name; ?></title>
    <link rel="stylesheet" href="style.css">
</head>
<body class="<?php echo $theme; ?>">
<div class="container">

    <h1><?php echo $name; ?></h1>
    <p><?php echo $desc; ?></p>

    <h2>Connect</h2>
    <div class="panel connect-box">
        <div class="host"><?php echo $host; ?></div>
        <div class="port-label">Port <?php echo $port; ?></div>
    </div>
    <p class="muted">
        Use any MUD client (Mudlet, TinTin++, MUSHclient) or telnet:<br>
        <code>telnet <?php echo $host . ' ' . $port; ?></code>
    </p>

    <h2>About</h2>
    <div class="panel">
        <p>
            Dystopia is a classless MUD where characters evolve through powerful
            supernatural forms &mdash; vampires, werewolves, demons, mages, and more.
            Choose your path, master unique abilities, and carve your legend in a
            dark cyberpunk world.
        </p>
    </div>

    <div class="footer">
        <a href="https://github.com/Coffee-Nerd/dystopia-mud">GitHub</a>
        &middot;
        Powered by Dystopia MUD Engine
    </div>

</div>
</body>
</html>
