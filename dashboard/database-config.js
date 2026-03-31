/**
 * BeeSense Database Configuration
 * 
 * Supports both SQLite (local/EC2) and MySQL (AWS RDS)
 */

const fs = require('fs');
const path = require('path');

// Database configuration
const config = {
    // Default: SQLite (for EC2 or local development)
    type: process.env.DB_TYPE || 'sqlite',
    
    // SQLite configuration
    sqlite: {
        path: process.env.DB_PATH || path.join(__dirname, 'beesense.db')
    },
    
    // MySQL/RDS configuration (for production)
    mysql: {
        host: process.env.DB_HOST || 'localhost',
        port: process.env.DB_PORT || 3306,
        user: process.env.DB_USER || 'beesense',
        password: process.env.DB_PASSWORD || '',
        database: process.env.DB_NAME || 'beesense',
        connectionLimit: 10
    }
};

/**
 * Create appropriate database instance based on configuration
 */
function createDatabase() {
    if (config.type === 'mysql') {
        console.log('📊 Using MySQL database (AWS RDS)');
        const MySQLDatabase = require('./database-mysql');
        return new MySQLDatabase(config.mysql);
    } else {
        console.log('📊 Using SQLite database');
        const SQLiteDatabase = require('./database');
        return new SQLiteDatabase(config.sqlite.path);
    }
}

module.exports = {
    config,
    createDatabase
};
