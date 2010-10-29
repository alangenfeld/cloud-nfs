
/*
 * BIGINT are signed integers on 8 bytes !
 */
CREATE TABLE Handle (
  handleId  BIGSERIAL, 
  handleTs  INT,
  deviceId  BIGINT NOT NULL, 
  inode     BIGINT NOT NULL, 
  ctime     INT, 
  nlink     SMALLINT DEFAULT 1,
  ftype     SMALLINT,
  PRIMARY KEY(handleId, handleTs),
  UNIQUE (deviceId, inode)
);

CREATE TABLE Parent (
  handleId        BIGINT NOT NULL,
  handleTs        INT NOT NULL,
  handleIdParent  BIGINT,
  handleTsParent  INT,
  name            TEXT,
  UNIQUE (handleIdParent, handleTsParent, name),
  FOREIGN KEY (handleId, handleTs) REFERENCES Handle(handleId, handleTs) ON DELETE CASCADE,
  FOREIGN KEY (handleidparent, handletsparent) REFERENCES handle(handleid, handlets) ON DELETE CASCADE
);
CREATE INDEX parent_handle_index ON Parent (handleId, HandleTs);

