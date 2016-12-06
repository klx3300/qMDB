/* ====================================================
 * qMDB Slave database
 * the master database do NOT store any data.
 * instead, the slave database stores all data.
 * the master database will have a scheduler to select which
 * insertion request will be sent to the slave database,
 * and the master database still have its ways to find
 * which server the data are stored in.
 * ===================================================
 */


