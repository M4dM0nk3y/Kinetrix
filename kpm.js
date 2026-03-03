#!/usr/bin/env node

/**
 * Kinetrix Package Manager (kpm)
 * Command-line tool for managing Kinetrix packages
 */

const fs = require('fs');
const path = require('path');
const https = require('https');

const VERSION = '1.0.0';
const REGISTRY_URL = 'https://packages.kinetrix.dev';
const STDLIB_PATH = path.join(__dirname, 'stdlib');

// Colors for terminal output
const colors = {
  reset: '\x1b[0m',
  green: '\x1b[32m',
  red: '\x1b[31m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  cyan: '\x1b[36m'
};

function log(message, color = 'reset') {
  console.log(`${colors[color]}${message}${colors.reset}`);
}

function error(message) {
  log(`✗ ${message}`, 'red');
}

function success(message) {
  log(`✓ ${message}`, 'green');
}

function info(message) {
  log(`ℹ ${message}`, 'cyan');
}

// Initialize new package
function init() {
  const packageJson = {
    name: path.basename(process.cwd()),
    version: '1.0.0',
    description: '',
    author: '',
    license: 'MIT',
    main: 'main.kx',
    dependencies: {},
    keywords: [],
    hardware: ['arduino-uno'],
    difficulty: 'beginner'
  };

  fs.writeFileSync('package.json', JSON.stringify(packageJson, null, 2));
  success('Created package.json');
  info('Edit package.json to customize your package');
}

// Install package
function install(packageName) {
  if (!packageName) {
    error('Please specify a package name');
    info('Usage: kpm install <package-name>');
    return;
  }

  info(`Installing ${packageName}...`);

  // Check if package exists in stdlib
  const packagePath = path.join(STDLIB_PATH, packageName + '.kx');
  
  if (fs.existsSync(packagePath)) {
    // Local install from stdlib
    const destPath = path.join(process.cwd(), 'kinetrix_modules', packageName + '.kx');
    const destDir = path.dirname(destPath);
    
    if (!fs.existsSync(destDir)) {
      fs.mkdirSync(destDir, { recursive: true });
    }
    
    fs.copyFileSync(packagePath, destPath);
    success(`Installed ${packageName} from local stdlib`);
    
    // Update package.json dependencies
    updateDependencies(packageName, '1.0.0');
  } else {
    // Try to fetch from registry (future implementation)
    error(`Package ${packageName} not found in local stdlib`);
    info('Remote registry support coming soon!');
  }
}

// Update package.json dependencies
function updateDependencies(packageName, version) {
  if (!fs.existsSync('package.json')) {
    return;
  }

  const packageJson = JSON.parse(fs.readFileSync('package.json', 'utf8'));
  
  if (!packageJson.dependencies) {
    packageJson.dependencies = {};
  }
  
  packageJson.dependencies[packageName] = `^${version}`;
  fs.writeFileSync('package.json', JSON.stringify(packageJson, null, 2));
}

// List installed packages
function list() {
  if (!fs.existsSync('package.json')) {
    error('No package.json found. Run "kpm init" first.');
    return;
  }

  const packageJson = JSON.parse(fs.readFileSync('package.json', 'utf8'));
  
  if (!packageJson.dependencies || Object.keys(packageJson.dependencies).length === 0) {
    info('No packages installed');
    return;
  }

  log('\nInstalled packages:', 'cyan');
  for (const [name, version] of Object.entries(packageJson.dependencies)) {
    log(`  ${name}@${version}`, 'green');
  }
}

// Search packages
function search(query) {
  if (!query) {
    error('Please specify a search query');
    return;
  }

  info(`Searching for "${query}"...`);

  // Search in local stdlib
  const results = [];
  
  function searchDir(dir, prefix = '') {
    const files = fs.readdirSync(dir);
    
    for (const file of files) {
      const fullPath = path.join(dir, file);
      const stat = fs.statSync(fullPath);
      
      if (stat.isDirectory()) {
        searchDir(fullPath, prefix + file + '/');
      } else if (file.endsWith('.kx')) {
        const name = prefix + file.replace('.kx', '');
        if (name.toLowerCase().includes(query.toLowerCase())) {
          results.push(name);
        }
      }
    }
  }

  if (fs.existsSync(STDLIB_PATH)) {
    searchDir(STDLIB_PATH);
  }

  if (results.length === 0) {
    info('No packages found');
  } else {
    log(`\nFound ${results.length} package(s):`, 'cyan');
    results.forEach(name => log(`  ${name}`, 'green'));
  }
}

// Show package info
function info_package(packageName) {
  const packagePath = path.join(STDLIB_PATH, packageName);
  const metadataPath = path.join(packagePath, 'package.json');

  if (fs.existsSync(metadataPath)) {
    const metadata = JSON.parse(fs.readFileSync(metadataPath, 'utf8'));
    
    log(`\n${metadata.name} v${metadata.version}`, 'cyan');
    log(`${metadata.description}\n`, 'reset');
    
    if (metadata.features) {
      log('Features:', 'yellow');
      metadata.features.forEach(f => log(`  • ${f}`, 'reset'));
    }
    
    if (metadata.hardware) {
      log('\nSupported Hardware:', 'yellow');
      log(`  ${metadata.hardware.join(', ')}`, 'reset');
    }
    
    log(`\nDifficulty: ${metadata.difficulty}`, 'yellow');
    log(`Complexity: ${metadata.complexity}/10`, 'yellow');
  } else {
    error(`Package ${packageName} not found`);
  }
}

// Show help
function help() {
  log('\nKinetrix Package Manager (kpm) v' + VERSION, 'cyan');
  log('\nUsage:', 'yellow');
  log('  kpm init                    Initialize new package');
  log('  kpm install <package>       Install a package');
  log('  kpm list                    List installed packages');
  log('  kpm search <query>          Search for packages');
  log('  kpm info <package>          Show package information');
  log('  kpm help                    Show this help message');
  log('  kpm version                 Show version\n');
}

// Main CLI
const args = process.argv.slice(2);
const command = args[0];

switch (command) {
  case 'init':
    init();
    break;
  case 'install':
  case 'i':
    install(args[1]);
    break;
  case 'list':
  case 'ls':
    list();
    break;
  case 'search':
    search(args[1]);
    break;
  case 'info':
    info_package(args[1]);
    break;
  case 'version':
  case '-v':
  case '--version':
    log(`kpm v${VERSION}`, 'cyan');
    break;
  case 'help':
  case '-h':
  case '--help':
  default:
    help();
    break;
}
