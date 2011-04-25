/*********************************************************
 * Module name: ftl_data.c
 *
 * Copyright 2010, 2011. All Rights Reserved, Crane Chu.
 *
 * This file is part of OpenNFM.
 *
 * OpenNFM is free software: you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any 
 * later version.
 * 
 * OpenNFM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with OpenNFM. If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * First written on 2010-01-01 by cranechu@gmail.com
 *
 * Module Description:
 *    data journal read/write.
 *
 *********************************************************/


#include <core\inc\cmn.h>
#include <core\inc\buf.h>
#include <core\inc\ubi.h>

#include <sys\sys.h>

#include "ftl_inc.h"


/* journal edition for orderly replay */
static UINT32     edition_in_hot_journal = 0;
static UINT32     edition_in_cold_journal = 0;

/* meta data */
static SPARE      hot_meta_data[JOURNAL_BLOCK_COUNT][PAGE_PER_PHY_BLOCK];
static SPARE      cold_meta_data[JOURNAL_BLOCK_COUNT][PAGE_PER_PHY_BLOCK];

/* buffer used in reclaim */
static SPARE      pages_buffer[MPP_SIZE/sizeof(SPARE)];
static UINT8      data_buffer[MPP_SIZE];
static LOG_BLOCK  dirty_blocks[JOURNAL_BLOCK_COUNT];


STATUS DATA_Format()
{
   UINT32      i;
   LOG_BLOCK   block = DATA_START_BLOCK;
   STATUS      ret = STATUS_SUCCESS;

   /* init the bdt to all dirty */
   for (i=0; i<CFG_LOG_BLOCK_COUNT; i++)
   {
      block_dirty_table[i] = MAX_DIRTY_PAGES;
   }

   /* init the journal blocks in root table */
   for (i=0; i<JOURNAL_BLOCK_COUNT; i++)
   {
      if (ret == STATUS_SUCCESS)
      {
         ret = UBI_Erase(block, block);
      }

      if (ret == STATUS_SUCCESS)
      {
         PM_NODE_SET_BLOCKPAGE(root_table.hot_journal[i], block, 0);
         block_dirty_table[block] = 0;
         block ++;
      }
   }

   for (i=0; i<JOURNAL_BLOCK_COUNT; i++)
   {
      if (ret == STATUS_SUCCESS)
      {
         ret = UBI_Erase(block, block);
      }

      if (ret == STATUS_SUCCESS)
      {
         PM_NODE_SET_BLOCKPAGE(root_table.cold_journal[i], block, 0);
         block_dirty_table[block] = 0;
         block ++;
      }
   }

   for (i=0; i<JOURNAL_BLOCK_COUNT; i++)
   {
      if (ret == STATUS_SUCCESS)
      {
         ret = UBI_Erase(block, block);
      }

      if (ret == STATUS_SUCCESS)
      {
         PM_NODE_SET_BLOCKPAGE(root_table.reclaim_journal[i], block, 0);
         block_dirty_table[block] = 0;
         block ++;
      }
   }

   return STATUS_SUCCESS;
}


STATUS DATA_Write(PGADDR addr, void* buffer, BOOL is_hot)
{
   UINT32         i;
   UINT32*        edition;
   PHY_BLOCK      block;
   PAGE_OFF       page;
   JOURNAL_ADDR*  data_journal;
   SPARE*         meta_data;
   STATUS         ret = STATUS_SUCCESS;

   /* TODO: optimize this critical path */
   /* TODO: Bad Page Marker, skip the bad PAGE instead of bad BLOCK. */

   if (is_hot == TRUE)
   {
      data_journal = root_table.hot_journal;
      meta_data = &(hot_meta_data[0][0]);
      edition = &edition_in_hot_journal;
   }
   else
   {
      data_journal = root_table.cold_journal;
      meta_data = &(cold_meta_data[0][0]);
      edition = &edition_in_cold_journal;
   }

   /* find an idle non-full block */
   do
   {
      for (i=0; i<JOURNAL_BLOCK_COUNT; i++)
      {
         if (PM_NODE_PAGE(data_journal[i]) < PAGE_PER_PHY_BLOCK-1)
         {
            ret = UBI_ReadStatus(PM_NODE_BLOCK(data_journal[i]));
            if (ret == STATUS_SUCCESS)
            {
               /* success means idle */
               data_journal = &data_journal[i];
               meta_data = meta_data+i*PAGE_PER_PHY_BLOCK;
               break;
            }
         }
      }
   } while (ret==STATUS_DIE_BUSY);

   ASSERT(ret==STATUS_SUCCESS);
   if (buffer != NULL)
   {
      block = PM_NODE_BLOCK(*data_journal);
      page = PM_NODE_PAGE(*data_journal);

      /* prepare spare data, and set in meta table */
      meta_data[page][0] = addr;
      meta_data[page][1] = (*edition);
      (*edition) = (*edition)+1;

      /* write the page to journal block */
      ret = UBI_Write(block, page, buffer, meta_data[page], TRUE);
      if (ret == STATUS_SUCCESS)
      {
         /* update PMT */
         ret = PMT_Update(addr, block, page);
      }

      if (ret == STATUS_SUCCESS)
      {
         /* update journal */
         PM_NODE_SET_BLOCKPAGE(*data_journal, block, page+1);
      }

      if (PM_NODE_PAGE(*data_journal) == PAGE_PER_PHY_BLOCK-1)
      {
         /* write meta data to last page */
         ret = UBI_Write(block, PAGE_PER_PHY_BLOCK-1, meta_data, NULL, FALSE);
      }
   }
   else
   {
      /* no buffer, so no need to write data. Just treat it as page trim. */
      /* update PMT */
      ret = PMT_Update(addr, INVALID_BLOCK, INVALID_PAGE);
   }

   return ret;
}


STATUS DATA_Commit()
{
   STATUS ret;

   ret = HDI_Commit();
   if (ret == STATUS_SUCCESS)
   {
      ret = PMT_Commit();
   }

   if (ret == STATUS_SUCCESS)
   {
      ret = BDT_Commit();
   }

   if (ret == STATUS_SUCCESS)
   {
      ret = ROOT_Commit();
   }

   if (ret == STATUS_SUCCESS)
   {
      edition_in_hot_journal = 0;
      edition_in_cold_journal = 0;
   }

   return ret;
}


BOOL DATA_IsFull(BOOL hot_journal)
{
   UINT32   i;
   BOOL     ret = TRUE;

   for (i=0; i<JOURNAL_BLOCK_COUNT; i++)
   {
      if (hot_journal == TRUE)
      {
         if (PM_NODE_PAGE(root_table.hot_journal[i]) < PAGE_PER_PHY_BLOCK-1)
         {
            ret = FALSE;
            break;
         }
      }
      else
      {
         if (PM_NODE_PAGE(root_table.cold_journal[i]) < PAGE_PER_PHY_BLOCK-1)
         {
            ret = FALSE;
            break;
         }
      }
   }

   return ret;
}


STATUS DATA_Reclaim(BOOL is_hot)
{
   UINT32         i, j;
   UINT32*        edition;
   UINT32         total_valid_page = 0;
   PAGE_OFF       next_dirty_count = 0;
   PAGE_OFF       target_dirty_count = MAX_DIRTY_PAGES;
   UINT32         found_block = 0;
   JOURNAL_ADDR*  journal;
   JOURNAL_ADDR*  exclude_journal;
   SPARE*         meta_data;
   UINT32         total_reclaimed_page = 0;
   LOG_BLOCK      reclaim_block;
   LOG_BLOCK      dirty_block;
   PAGE_OFF       reclaim_page = 0;
   PAGE_OFF       page;
   SPARE*         meta_data_buffer;
   LOG_BLOCK      true_block = INVALID_BLOCK;
   PAGE_OFF       true_page = INVALID_PAGE;
   SPARE          spare;
   STATUS         ret = STATUS_SUCCESS;

   if (is_hot == TRUE)
   {
      journal = root_table.hot_journal;
      exclude_journal = root_table.cold_journal;
      meta_data = &(hot_meta_data[0][0]);
      edition = &edition_in_hot_journal;
   }
   else
   {
      journal = root_table.cold_journal;
      exclude_journal = root_table.hot_journal;
      meta_data = &(cold_meta_data[0][0]);
      edition = &edition_in_cold_journal;
   }

   /* data reclaim process:
    * - flush and release all write buffer
    * - find the dirtiest blocks.
    * - copy valid pages in dirty blocks to reclaim blocks,
    * - update PMT and reclaim journal (keep integrity for PLR)
    * - erase dirtiest blocks, assign to new low EC blocks in same die
    * - update journals: reclaim ==> journal, dirty ==> reclaim
    */

   /* flush and release all write buffer */
   if (ret == STATUS_SUCCESS)
   {
      ret = UBI_Flush();
   }

   /* find the dirtiest blocks */
   if (ret == STATUS_SUCCESS)
   {
      while (found_block != JOURNAL_BLOCK_COUNT)
      {
         for (i=DATA_START_BLOCK; i<=DATA_LAST_BLOCK; i++)
         {
            /* exclude journal blocks */
            for (j=0; j<JOURNAL_BLOCK_COUNT; j++)
            {
               if (i == PM_NODE_BLOCK(exclude_journal[j]))
               {
                  /* skip the journal block */
                  break;
               }
            }

            if (j < JOURNAL_BLOCK_COUNT)
            {
               continue;
            }

            if (block_dirty_table[i] == target_dirty_count)
            {
               dirty_blocks[found_block] = i;
               total_valid_page += (MAX_DIRTY_PAGES-block_dirty_table[i]);
               found_block ++;
            }
            else
            {
               /* set the next target dirty count */
               if (block_dirty_table[i] < target_dirty_count &&
                   block_dirty_table[i] > next_dirty_count)
               {
                  next_dirty_count = block_dirty_table[i];
               }
            }

            if (found_block == JOURNAL_BLOCK_COUNT)
            {
               break;
            }
         }

         /* find more dirty blocks */
         target_dirty_count = next_dirty_count;
         next_dirty_count = 0;
      }
   }

   if (ret == STATUS_SUCCESS)
   {
      if (total_valid_page != 0)
      {
         for (j=0; j<JOURNAL_BLOCK_COUNT; j++)
         {
            /* copy valid pages in dirty blocks to reclaim blocks */
            /* keep integrity before PMT_Update() */
            reclaim_block = PM_NODE_BLOCK(root_table.reclaim_journal[j]);
            reclaim_page = 0;
            dirty_block = dirty_blocks[j];
            meta_data_buffer = meta_data + j*PAGE_PER_PHY_BLOCK;

            ret = UBI_Read(dirty_block, PAGE_PER_PHY_BLOCK-1, pages_buffer, NULL);
            if (ret == STATUS_SUCCESS)
            {
               for (page=0; page<PAGE_PER_PHY_BLOCK-1; page++)
               {
                  if (ret == STATUS_SUCCESS)
                  {
                     ret = PMT_Search(pages_buffer[page][0],
                                      &true_block,
                                      &true_page);
                  }

                  if (ret == STATUS_SUCCESS)
                  {
                     if (true_block == dirty_block && true_page == page)
                     {
                        /* this page is valid */
                        /* copy valid page to reclaim block */
                        ret = UBI_Read(dirty_block, page, data_buffer, spare);
                        if (ret == STATUS_SUCCESS)
                        {
                           /* logical page address is not changed */
                           spare[1] = total_reclaimed_page;

                           ret = UBI_Write(reclaim_block,
                                           reclaim_page,
                                           data_buffer,
                                           spare,
                                           FALSE);
                        }

                        if (ret == STATUS_SUCCESS)
                        {
                           /* update pmt */
                           ret = PMT_Update(pages_buffer[page][0],
                                            reclaim_block,
                                            reclaim_page);

                           /* check logical page address */
                           ASSERT(spare[0] == pages_buffer[page][0]);
                           /* check edition number */
                           ASSERT(spare[1] == total_reclaimed_page);

                           /* update meta data */
                           meta_data_buffer[reclaim_page][0] = spare[0];
                           meta_data_buffer[reclaim_page][1] = spare[1];
                        }

                        if (ret == STATUS_SUCCESS)
                        {
                           reclaim_page ++;
                           total_reclaimed_page ++;

                           /* update journals */
                           PM_NODE_SET_BLOCKPAGE(root_table.reclaim_journal[j],
                                                 reclaim_block, reclaim_page);
                        }
                     }
                  }
               }
            }
         }

         ASSERT(total_valid_page == total_reclaimed_page);

         /* copied all valid page in all dirty blocks.
          * Erase dirtiest blocks, assign to new low EC blocks in different
          * dice, and update journals: reclaim ==> journal, dirty ==> reclaim
          */
         for (j=0; j<JOURNAL_BLOCK_COUNT; j++)
         {
            if (ret == STATUS_SUCCESS)
            {
               ret = UBI_Erase(dirty_blocks[j], j);
            }

            if (ret == STATUS_SUCCESS)
            {
               journal[j] = root_table.reclaim_journal[j];
               PM_NODE_SET_BLOCKPAGE(root_table.reclaim_journal[j],
                                     dirty_blocks[j], 0);

               /* update blocks: origin journal - not changed
                *                origin dirty   - clear all dirty
                *                origin reclaim - not changed, and shoudl be 0
                */
               block_dirty_table[dirty_blocks[j]] = 0;
               block_dirty_table[PM_NODE_BLOCK(journal[j])] = 0;
            }
         }
      }
      else
      {
         /* erase dirty blocks, update journals, and no need to copy pages */
         for (j=0; j<JOURNAL_BLOCK_COUNT; j++)
         {
            if (ret == STATUS_SUCCESS)
            {
               ret = UBI_Erase(dirty_blocks[j], j);
            }

            if (ret == STATUS_SUCCESS)
            {
               /* dirty ==> journal, reclaim unchanged */
               PM_NODE_SET_BLOCKPAGE(journal[j], dirty_blocks[j], 0);

               /* BDT: clear dirty (now journal) */
               block_dirty_table[dirty_blocks[j]] = 0;
            }
         }
      }
   }

   if (ret == STATUS_SUCCESS)
   {
      (*edition) = total_valid_page;
   }

   return ret;
}


STATUS DATA_Replay(JOURNAL_ADDR* journals)
{
   UINT32      journal_edition = 0;
   UINT32      j_index = 0;
   SPARE*      meta_data;
   LOG_BLOCK   block;
   PAGE_OFF    page;
   SPARE       spare;
   UINT32      page_edition;
   PGADDR      logical_page_address;
   SPARE*      meta_data_buffer;
   STATUS      ret = STATUS_SUCCESS;

   if (journals == root_table.hot_journal)
   {
      meta_data = &(hot_meta_data[0][0]);
   }
   else
   {
      meta_data = &(cold_meta_data[0][0]);
   }

   while (journal_edition != MAX_UINT32)
   {
      for (j_index=0; j_index<JOURNAL_BLOCK_COUNT; j_index++)
      {
         block = PM_NODE_BLOCK(journals[j_index]);
         page = PM_NODE_PAGE(journals[j_index]);

         if (ret == STATUS_SUCCESS)
         {
            ret = UBI_Read(block, page, NULL, spare);
         }

         if (ret == STATUS_SUCCESS)
         {
            /* this page was written, replay it */
            logical_page_address = spare[0];
            page_edition = spare[1];

            if (page_edition != journal_edition)
            {
               /* replay pages in the edition order */
               continue;
            }

            /* update PMT */
            ret = PMT_Update(logical_page_address, block, page);
         }

         if (ret == STATUS_SUCCESS)
         {
            /* update journal */
            PM_NODE_SET_BLOCKPAGE(journals[j_index], block, page+1);

            /* find next edition of journal */
            journal_edition ++;
            break;
         }
         else
         {
            /* restore the edition */
            if (journals == root_table.hot_journal)
            {
               edition_in_hot_journal = journal_edition;
            }
            else
            {
               edition_in_cold_journal = journal_edition;
            }

            /* empty page in this journal block */
            journal_edition = MAX_UINT32;
            ret = STATUS_SUCCESS;
            continue;
         }
      }
   }

   /* build up the meta table */
   for (j_index=0; j_index<JOURNAL_BLOCK_COUNT; j_index++)
   {
      block = PM_NODE_BLOCK(journals[j_index]);

      /* point to the right meta data address */
      meta_data_buffer= meta_data + j_index*PAGE_PER_PHY_BLOCK;

      for (page=0; page<PAGE_PER_PHY_BLOCK; page++)
      {
         if (ret == STATUS_SUCCESS)
         {
            ret = UBI_Read(block, page, NULL, spare);
         }

         if (ret == STATUS_SUCCESS)
         {
            meta_data_buffer[page][0] = spare[0];
            meta_data_buffer[page][1] = spare[1];
         }
         else if (page == PAGE_PER_PHY_BLOCK-1)
         {
            /* write meta data to last page */
            ret = UBI_Write(block,
                            PAGE_PER_PHY_BLOCK-1,
                            meta_data_buffer,
                            NULL,
                            FALSE);
         }
         else
         {
            ret = STATUS_SUCCESS;
            break;
         }
      }
   }

   return ret;
}


