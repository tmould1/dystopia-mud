<?php
/**
 * Dystopia MUD — Admin Panel
 *
 * Authentication via MUD player SQLite databases.
 * Requires immortal level (7+) to access.
 *
 * Compatible with PHP 5.3+.
 */

// -- Session setup (secure cookies) ------------------------------------------

// Set secure cookie params before session_start (PHP 5.3 compatible)
ini_set('session.cookie_httponly', '1');
if (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') {
    ini_set('session.cookie_secure', '1');
}
ini_set('session.use_only_cookies', '1');

session_start();

// -- Constants ----------------------------------------------------------------

define('LEVEL_IMMORTAL', 7);
define('SESSION_TIMEOUT', 1800); // 30 minutes
define('MAX_LOGIN_DELAY', 4);    // seconds, max delay after failures

$config_file = dirname(__FILE__) . '/config.json';

// -- Helper functions ---------------------------------------------------------

function load_config() {
    global $config_file;
    $defaults = array(
        'mud_name'       => 'Dystopia MUD',
        'hostname'       => 'coming.soon',
        'port'           => 8000,
        'description'    => 'A dark cyberpunk text-based multiplayer game.',
        'theme'          => 'cyberpunk',
        'player_db_path' => '/home/mud/fatu/edition/gamedata/db/players',
    );
    if (file_exists($config_file)) {
        $json = file_get_contents($config_file);
        if ($json !== false) {
            $parsed = json_decode($json, true);
            if (is_array($parsed)) {
                return array_merge($defaults, $parsed);
            }
        }
    }
    return $defaults;
}

function save_config($config) {
    global $config_file;
    // json_encode with pretty print (PHP 5.4+), fallback for 5.3
    if (defined('JSON_PRETTY_PRINT')) {
        $json = json_encode($config, JSON_PRETTY_PRINT);
    } else {
        $json = json_encode($config);
    }
    return file_put_contents($config_file, $json . "\n") !== false;
}

function generate_csrf() {
    if (empty($_SESSION['csrf_token'])) {
        // PHP 5.3 compatible random token
        $_SESSION['csrf_token'] = md5(uniqid(mt_rand(), true));
    }
    return $_SESSION['csrf_token'];
}

function verify_csrf($token) {
    return isset($_SESSION['csrf_token']) && $token === $_SESSION['csrf_token'];
}

function check_session_timeout() {
    if (isset($_SESSION['last_activity'])) {
        if (time() - $_SESSION['last_activity'] > SESSION_TIMEOUT) {
            session_destroy();
            return false;
        }
    }
    $_SESSION['last_activity'] = time();
    return true;
}

function get_login_delay() {
    $failures = isset($_SESSION['login_failures']) ? $_SESSION['login_failures'] : 0;
    return min($failures, MAX_LOGIN_DELAY);
}

function h($str) {
    return htmlspecialchars($str, ENT_QUOTES, 'UTF-8');
}

// -- Authentication -----------------------------------------------------------

function authenticate($name, $password, $player_db_path) {
    // Normalize name: ucfirst, lowercase rest
    $name = ucfirst(strtolower(trim($name)));

    if ($name === '' || $password === '') {
        return array('error' => 'Name and password are required.');
    }

    // Sanitize filename (alphanumeric only)
    if (!preg_match('/^[A-Za-z]+$/', $name)) {
        return array('error' => 'Invalid character name.');
    }

    $db_path = rtrim($player_db_path, '/') . '/' . $name . '.db';

    if (!file_exists($db_path)) {
        return array('error' => 'Character not found.');
    }

    try {
        $pdo = new PDO('sqlite:' . $db_path);
        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

        $stmt = $pdo->query('SELECT password, level, trust FROM player LIMIT 1');
        $row = $stmt->fetch(PDO::FETCH_ASSOC);

        if (!$row) {
            return array('error' => 'Character data not found.');
        }

        // Verify password: crypt(input, name) or plain-text match
        $stored = $row['password'];
        $crypt_hash = crypt($password, $name);
        $valid = ($crypt_hash === $stored) || ($password === $stored);

        if (!$valid) {
            return array('error' => 'Invalid password.');
        }

        $level = (int) $row['level'];
        if ($level < LEVEL_IMMORTAL) {
            return array('error' => 'Access denied. Immortal level required.');
        }

        return array(
            'name'  => $name,
            'level' => $level,
        );
    } catch (Exception $e) {
        return array('error' => 'Could not read player database.');
    }
}

// -- Load config and theme ----------------------------------------------------

$config = load_config();
$theme = preg_replace('/[^a-z]/', '', strtolower($config['theme']));

$available_themes = array(
    'cyberpunk'  => 'Cyberpunk',
    'medieval'   => 'Medieval',
    'shadow'     => 'Shadow',
    'nature'     => 'Nature',
    'inferno'    => 'Inferno',
    'ocean'      => 'Ocean',
    'arctic'     => 'Arctic',
    'wasteland'  => 'Wasteland',
);

// -- Handle logout ------------------------------------------------------------

if (isset($_GET['logout'])) {
    session_destroy();
    header('Location: admin.php');
    exit;
}

// -- Check session ------------------------------------------------------------

$logged_in = false;
$message = '';
$msg_type = '';

if (isset($_SESSION['admin_name'])) {
    if (!check_session_timeout()) {
        $message = 'Session expired. Please log in again.';
        $msg_type = 'error';
    } else {
        $logged_in = true;
    }
}

// -- Handle POST: login or config save ----------------------------------------

if ($_SERVER['REQUEST_METHOD'] === 'POST') {

    if (isset($_POST['action']) && $_POST['action'] === 'login') {
        // Rate limiting: delay based on prior failures
        $delay = get_login_delay();
        if ($delay > 0) {
            sleep($delay);
        }

        $name = isset($_POST['name']) ? $_POST['name'] : '';
        $password = isset($_POST['password']) ? $_POST['password'] : '';

        $result = authenticate($name, $password, $config['player_db_path']);

        if (isset($result['error'])) {
            $message = $result['error'];
            $msg_type = 'error';
            $_SESSION['login_failures'] = get_login_delay() + 1;
        } else {
            $_SESSION['admin_name'] = $result['name'];
            $_SESSION['admin_level'] = $result['level'];
            $_SESSION['last_activity'] = time();
            $_SESSION['login_failures'] = 0;
            $_SESSION['csrf_token'] = '';
            $logged_in = true;
            $message = 'Logged in as ' . h($result['name']) . '.';
            $msg_type = 'success';
        }
    }

    if (isset($_POST['action']) && $_POST['action'] === 'save' && $logged_in) {
        // Verify CSRF
        $token = isset($_POST['csrf_token']) ? $_POST['csrf_token'] : '';
        if (!verify_csrf($token)) {
            $message = 'Invalid form token. Please try again.';
            $msg_type = 'error';
        } else {
            // Update editable fields
            $config['mud_name']    = trim($_POST['mud_name']);
            $config['hostname']    = trim($_POST['hostname']);
            $config['port']        = (int) $_POST['port'];
            $config['description'] = trim($_POST['description']);
            $config['theme']       = trim($_POST['theme']);
            // player_db_path is NOT editable from web UI

            if (save_config($config)) {
                $message = 'Configuration saved.';
                $msg_type = 'success';
                $theme = preg_replace('/[^a-z]/', '', strtolower($config['theme']));
            } else {
                $message = 'Failed to save configuration. Check file permissions.';
                $msg_type = 'error';
            }

            // Regenerate CSRF token after successful save
            $_SESSION['csrf_token'] = '';
        }
    }
}

$csrf = generate_csrf();

?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Admin &mdash; <?php echo h($config['mud_name']); ?></title>
    <link rel="stylesheet" href="style.css">
</head>
<body class="<?php echo $theme; ?>">
<div class="container">

    <h1><?php echo h($config['mud_name']); ?> &mdash; Admin</h1>

    <?php if ($message): ?>
    <div class="msg msg-<?php echo $msg_type; ?>"><?php echo h($message); ?></div>
    <?php endif; ?>

    <?php if (!$logged_in): ?>
    <!-- ============ Login Form ============ -->
    <div class="panel">
        <h2>Login</h2>
        <p class="muted">Sign in with your MUD character (Immortal+).</p>
        <form method="post" action="admin.php">
            <input type="hidden" name="action" value="login">
            <label for="name">Character Name</label>
            <input type="text" id="name" name="name" required autocomplete="username">
            <label for="password">Password</label>
            <input type="password" id="password" name="password" required autocomplete="current-password">
            <div class="btn-row">
                <button type="submit" class="btn">Log In</button>
                <a href="index.php" class="muted">&larr; Back</a>
            </div>
        </form>
    </div>

    <?php else: ?>
    <!-- ============ Config Editor ============ -->
    <p class="muted">
        Logged in as <strong><?php echo h($_SESSION['admin_name']); ?></strong>
        (level <?php echo (int) $_SESSION['admin_level']; ?>)
        &mdash; <a href="admin.php?logout=1">Logout</a>
    </p>

    <div class="panel">
        <h2>Site Settings</h2>
        <form method="post" action="admin.php">
            <input type="hidden" name="action" value="save">
            <input type="hidden" name="csrf_token" value="<?php echo h($csrf); ?>">

            <label for="mud_name">MUD Name</label>
            <input type="text" id="mud_name" name="mud_name"
                   value="<?php echo h($config['mud_name']); ?>" required>

            <label for="hostname">Hostname</label>
            <input type="text" id="hostname" name="hostname"
                   value="<?php echo h($config['hostname']); ?>" required>

            <label for="port">Port</label>
            <input type="number" id="port" name="port"
                   value="<?php echo (int) $config['port']; ?>" min="1" max="65535" required>

            <label for="description">Description</label>
            <textarea id="description" name="description" rows="3"><?php echo h($config['description']); ?></textarea>

            <label for="theme">Theme</label>
            <select id="theme" name="theme">
                <?php foreach ($available_themes as $key => $label): ?>
                <option value="<?php echo $key; ?>"<?php if ($config['theme'] === $key) echo ' selected'; ?>><?php echo $label; ?></option>
                <?php endforeach; ?>
            </select>

            <div class="btn-row">
                <button type="submit" class="btn">Save</button>
            </div>
        </form>
    </div>

    <div class="footer">
        <a href="index.php">&larr; View public page</a>
    </div>

    <?php endif; ?>

</div>
</body>
</html>
